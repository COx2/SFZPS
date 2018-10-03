#include "SFZVoice.h"
#include "SFZSound.h"
#include "SFZRegion.h"
#include "SFZSample.h"
#include "SFZDebug.h"
#include <math.h>

#include "../Source/DSP/SynthParameters.h"

using namespace SFZero;

static const float globalGain = -1.0;

#undef M_PI
#define M_PI 3.14159265358979323846

// The lower this block size is the more accurate the effects are.
// Increasing the value significantly lowers the CPU usage of the voice rendering.
// If LFO affects the lowpass filter it can be hearable even as low as 8.
enum { SFZVOICE_RENDER_EFFECTSAMPLEBLOCK = 64 };


void SFZVoice::SFZLowpass::setup(float Fc)
{
	// Lowpass filter from http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
	double K = tan(M_PI * Fc), KK = K * K;
	double norm = 1 / (1 + K * QInv + KK);
	a0 = KK * norm;
	a1 = 2 * a0;
	b1 = 2 * (KK - 1) * norm;
	b2 = (1 - K * QInv + KK) * norm;
}


inline float SFZVoice::SFZLowpass::process(double In, double& z1, double& z2)
{
	double Out = In * a0 + z1; z1 = In * a1 + z2 - b1 * Out; z2 = In * a0 - b2 * Out; return (float)Out;
}


void SFZVoice::SFZLFO::setup(float delay, int freqCents, float sampleRate)
{
	samplesUntil = (long)(delay * sampleRate);
	delta = (4.0f * SFZRegion::cents2Hertz((float)freqCents) / sampleRate);
	level = 0;
}


inline void SFZVoice::SFZLFO::process(int blockSamples)
{
	if (samplesUntil > blockSamples)
		samplesUntil -= blockSamples;
	else {
		level += delta * blockSamples;
		if      (level >  1.0f) { delta = -delta; level =  2.0f - level; }
		else if (level < -1.0f) { delta = -delta; level = -2.0f - level; }
		}
}


SFZVoice::SFZVoice()
	: region(NULL)
{
	ampeg.setExponentialDecay(true);
	modeg.setExponentialDecay(false);
}


SFZero::SFZVoice::SFZVoice(AmpEGParameters * ampEGParams)
	: region(NULL)
	, _ampEGParamsPtr(ampEGParams)
{
	ampeg.setExponentialDecay(true);
	modeg.setExponentialDecay(false);
}


SFZVoice::~SFZVoice()
{
}


bool SFZVoice::canPlaySound(SynthesiserSound* sound)
{
	return dynamic_cast<SFZSound*>(sound) != NULL;
}


void SFZVoice::startNote(const int midiNoteNumber, const float floatVelocity, SynthesiserSound* soundIn, const int currentPitchWheelPosition)
{
	SFZSound* sound = dynamic_cast<SFZSound*>(soundIn);
	if (sound == NULL) {
		killNote();
		return;
		}

	int velocity = (int) (floatVelocity * 127.0);
	curVelocity = velocity;
	if (region == NULL)
		region = sound->getRegionFor(midiNoteNumber, velocity);
	if (region == NULL || region->sample == NULL || region->sample->getBuffer() == NULL) {
		killNote();
		return;
		}
	if (region->negative_end) {
		killNote();
		return;
		}

	// MOD: AMPEG Update.
	region->ampeg.attack = _ampEGParamsPtr->Attack->get();
	region->ampeg.decay = _ampEGParamsPtr->Decay->get();
	region->ampeg.sustain = _ampEGParamsPtr->Sustain->get();
	region->ampeg.release = _ampEGParamsPtr->Release->get();

	// Pitch.
	curMidiNote = midiNoteNumber;
	curPitchWheel = currentPitchWheelPosition;
	calcPitchRatio();

	// Gain.
	noteGainDB = globalGain + region->volume;
	// Thanks to <http:://www.drealm.info/sfz/plj-sfz.xhtml> for explaining the
	// velocity curve in a way that I could understand, although they mean
	// "log10" when they say "log".
	double velocityGainDB = -20.0 * log10((127.0 * 127.0) / (velocity * velocity));
	velocityGainDB *= region->amp_veltrack / 100.0;
	noteGainDB += (float)velocityGainDB;
	// The SFZ spec is silent about the pan curve, but a 3dB pan law seems
	// common.  This sqrt() curve matches what Dimension LE does; Alchemy Free
	// seems closer to sin(adjustedPan * pi/2).
	double adjustedPan = (region->pan + 100.0) / 200.0;
	panFactorLeft = (float)sqrt(1.0 - adjustedPan);
	panFactorRight = (float)sqrt(adjustedPan);
	ampeg.startNote(&region->ampeg, midiNoteNumber, floatVelocity, getSampleRate(), &region->ampeg_veltrack);

	// Offset/end.
	sourceSamplePosition = region->offset;
	sampleEnd = region->sample->sampleLength;
	if (region->end > 0 && region->end < sampleEnd)
		sampleEnd = region->end + 1;

	// Loop.
	loopStart = loopEnd = 0;
	SFZRegion::LoopMode loopMode = region->loop_mode;
	if (loopMode == SFZRegion::sample_loop) {
		if (region->sample->loopStart < region->sample->loopEnd)
			loopMode = SFZRegion::loop_continuous;
		else
			loopMode = SFZRegion::no_loop;
		}
	if (loopMode != SFZRegion::no_loop && loopMode != SFZRegion::one_shot) {
		if (region->loop_start < region->loop_end) {
			loopStart = region->loop_start;
			loopEnd = region->loop_end;
			}
		else {
			loopStart = region->sample->loopStart;
			loopEnd = region->sample->loopEnd;
			}
		}
	numLoops = 0;

	modeg.startNote(&region->modeg, midiNoteNumber, floatVelocity, getSampleRate(), NULL);

	// Setup lowpass filter.
	float filterQDB = region->initialFilterQ / 10.0f;
	lowpass.QInv = 1.0 / pow(10.0, (filterQDB / 20.0));
	lowpass.Z1Left = lowpass.Z2Left = lowpass.Z1Right = lowpass.Z2Right = 0;
	lowpass.active = (region->initialFilterFc <= 13500);
	if (lowpass.active) lowpass.setup(SFZRegion::cents2Hertz((float)region->initialFilterFc) / getSampleRate());

	// Setup LFO filters.
	modlfo.setup(region->delayModLFO, region->freqModLFO, getSampleRate());
	viblfo.setup(region->delayVibLFO, region->freqVibLFO, getSampleRate());
}


void SFZVoice::stopNote(float velocity, const bool allowTailOff)
{
	if (!allowTailOff || region == NULL) {
		killNote();
		return;
		}

	if (region->loop_mode != SFZRegion::one_shot) {
		ampeg.noteOff();
		modeg.noteOff();
		}
	if (region->loop_mode == SFZRegion::loop_sustain) {
		// Continue playing, but stop looping.
		loopEnd = loopStart;
		}
}


void SFZVoice::stopNoteForGroup()
{
	if (region->off_mode == SFZRegion::fast) {
		ampeg.fastRelease();
		modeg.fastRelease();
		}
	else {
		ampeg.noteOff();
		modeg.noteOff();
		}
}


void SFZVoice::stopNoteQuick()
{
	ampeg.fastRelease();
	modeg.fastRelease();
}


void SFZVoice::pitchWheelMoved(const int newValue)
{
	if (region == NULL)
		return;

	curPitchWheel = newValue;
	calcPitchRatio();
}


void SFZVoice::controllerMoved(
	const int controllerNumber,
	const int newValue)
{
	/***/
}


void SFZVoice::renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples)
{
	if (region == NULL)
		return;

	juce::AudioSampleBuffer* buffer = region->sample->getBuffer();
	const float* inL = buffer->getReadPointer(0, 0);
	const float* inR = buffer->getNumChannels() > 1 ? buffer->getReadPointer(1, 0) : NULL;

	float* outL = outputBuffer.getWritePointer(0, startSample);
	float* outR = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer(1, startSample) : NULL;

	// Cache some values, to give them at least some chance of ending up in
	// registers.
	const bool updateModEg       = (region->modEnvToPitch || region->modEnvToFilterFc);
	const bool updateModLFO      = (modlfo.delta && (region->modLfoToPitch || region->modLfoToFilterFc || region->modLfoToVolume));
	const bool updateVibLFO      = (viblfo.delta && (region->vibLfoToPitch));
	const bool dynamicLowpass    = (region->modLfoToFilterFc || region->modEnvToFilterFc);
	const bool dynamicPitchRatio = (region->modLfoToPitch || region->modEnvToPitch || region->vibLfoToPitch);
	const bool dynamicGain       = (region->modLfoToVolume != 0);
	const unsigned long tmpLoopStart = loopStart, tmpLoopEnd = loopEnd;
	const double tmpSampleEnd = (double)sampleEnd;
	double tmpSourceSamplePosition = sourceSamplePosition;
	SFZLowpass tmpLowpass = lowpass;

	float tmpSampleRate, tmpInitialFilterFc, tmpModLfoToFilterFc, tmpModEnvToFilterFc;
	if (dynamicLowpass)
		tmpSampleRate = getSampleRate(), tmpInitialFilterFc = (float)region->initialFilterFc, tmpModLfoToFilterFc = (float)region->modLfoToFilterFc, tmpModEnvToFilterFc = (float)region->modEnvToFilterFc;

	double pitchRatio;
	float tmpModLfoToPitch, tmpVibLfoToPitch, tmpModEnvToPitch;
	if (dynamicPitchRatio)
		tmpModLfoToPitch = (float)region->modLfoToPitch, tmpVibLfoToPitch = (float)region->vibLfoToPitch, tmpModEnvToPitch = (float)region->modEnvToPitch;
	else
		pitchRatio = SFZRegion::timecents2Secs(pitchInputTimecents) * pitchOutputFactor;

	float noteGain, tmpModLfoToVolume;
	if (dynamicGain)
		tmpModLfoToVolume = (float)region->modLfoToVolume * 0.1f;
	else
		noteGain = Decibels::decibelsToGain(noteGainDB);

	while (numSamples) {
		int blockSamples = (numSamples > SFZVOICE_RENDER_EFFECTSAMPLEBLOCK ? SFZVOICE_RENDER_EFFECTSAMPLEBLOCK : numSamples);
		numSamples -= blockSamples;

		if (dynamicLowpass) {
			float fres = tmpInitialFilterFc + modlfo.level * tmpModLfoToFilterFc + modeg.level * tmpModEnvToFilterFc;
			tmpLowpass.active = (fres <= 13500);
			if (tmpLowpass.active) tmpLowpass.setup(SFZRegion::cents2Hertz(fres) / tmpSampleRate);
			}

		if (dynamicPitchRatio)
			pitchRatio = SFZRegion::timecents2Secs(pitchInputTimecents + modlfo.level * tmpModLfoToPitch + viblfo.level * tmpVibLfoToPitch + modeg.level * tmpModEnvToPitch) * pitchOutputFactor;

		if (dynamicGain)
			noteGain = Decibels::decibelsToGain(noteGainDB + (modlfo.level * tmpModLfoToVolume));

		float gainMono = noteGain * ampeg.level, gainLeft = gainMono * panFactorLeft, gainRight = gainMono * panFactorRight;

		// Update EG.

		// MOD: AMPEG Update.
		region->ampeg.attack = _ampEGParamsPtr->Attack->get();
		region->ampeg.decay = _ampEGParamsPtr->Decay->get();
		region->ampeg.sustain = _ampEGParamsPtr->Sustain->get();
		region->ampeg.release = _ampEGParamsPtr->Release->get();

		ampeg.update(blockSamples);
		if (updateModEg)
			modeg.update(blockSamples);

		// Update LFOs.
		if (updateModLFO)
			modlfo.process(blockSamples);
		if (updateVibLFO)
			viblfo.process(blockSamples);

		while (blockSamples-- && tmpSourceSamplePosition < tmpSampleEnd) {
			unsigned long pos = (int)tmpSourceSamplePosition, nextPos = pos + 1;
			float alpha = (float)(tmpSourceSamplePosition - pos), invAlpha = 1.0f - alpha;
			if (tmpLoopStart < tmpLoopEnd && nextPos > tmpLoopEnd)
				nextPos = tmpLoopStart;

			// Simple linear interpolation.
			float l = (inL[pos] * invAlpha + inL[nextPos] * alpha);

			//low pass filter
			if (tmpLowpass.active)
				l = tmpLowpass.process(l, tmpLowpass.Z1Left, tmpLowpass.Z2Left);

			float r;
			if (inR) {
				r = (inR[pos] * invAlpha + inR[nextPos] * alpha);
				if (tmpLowpass.active)
					r = tmpLowpass.process(r, tmpLowpass.Z1Right, tmpLowpass.Z2Right);
				}
			else
				r = l;

			l *= gainLeft;
			r *= gainRight;
			// Shouldn't we dither here?

			if (outR) {
				*outL++ += l;
				*outR++ += r;
				}
			else
				*outL++ += (l + r) * 0.5f;

			// Next sample.
			tmpSourceSamplePosition += pitchRatio;
			if (tmpLoopStart < tmpLoopEnd && tmpSourceSamplePosition > tmpLoopEnd) {
				tmpSourceSamplePosition = tmpLoopStart;
				numLoops += 1;
				}

			}

		if (tmpSourceSamplePosition >= tmpSampleEnd || ampeg.isDone()) {
			killNote();
			break;
			}
		}

	sourceSamplePosition = tmpSourceSamplePosition;
	if (tmpLowpass.active || dynamicLowpass)
		lowpass = tmpLowpass;
}


bool SFZVoice::isPlayingNoteDown()
{
	return (region && region->trigger != SFZRegion::release);
}


bool SFZVoice::isPlayingOneShot()
{
	return (region && region->loop_mode == SFZRegion::one_shot);
}


int SFZVoice::getGroup()
{
	return (region ? region->group : 0);
}


int SFZVoice::getOffBy()
{
	return (region ? region->off_by : 0);
}


void SFZVoice::setRegion(SFZRegion* nextRegion)
{
	region = nextRegion;
}


juce::String SFZVoice::infoString()
{
	const char* egSegmentNames[] = {
		"delay", "attack", "hold", "decay", "sustain", "release", "done"
		};
	#define numEGSegments (sizeof(egSegmentNames) / sizeof(egSegmentNames[0]))
	const char* egSegmentName = "-Invalid-";
	int egSegmentIndex = ampeg.segmentIndex();
	if (egSegmentIndex >= 0 && egSegmentIndex < numEGSegments)
		egSegmentName = egSegmentNames[egSegmentIndex];

	char str[128];
	snprintf(
		str, 128,
		"note: %d, vel: %d, pan: %g, eg: %s, loops: %lu",
		curMidiNote, curVelocity, region->pan, egSegmentName, numLoops);
	return juce::String(str);
}



void SFZVoice::calcPitchRatio()
{
	double note = curMidiNote;
	note += region->transpose;
	note += region->tune / 100.0;

	double sampleRate = getSampleRate();
	double adjustedPitch =
		region->pitch_keycenter +
		(note - region->pitch_keycenter) * (region->pitch_keytrack / 100.0);
	if (curPitchWheel != 8192) {
		double wheel = ((2.0 * curPitchWheel / 16383.0) - 1.0);
		if (wheel > 0)
			adjustedPitch += wheel * region->bend_up / 100.0;
		else
			adjustedPitch += wheel * region->bend_down / -100.0;
		}
	pitchInputTimecents = adjustedPitch * 100.0;
	pitchOutputFactor = region->sample->getSampleRate() / (SFZRegion::timecents2Secs(region->pitch_keycenter * 100.0) * sampleRate);
}


void SFZVoice::killNote()
{
	region = NULL;
	clearCurrentNote();
}



