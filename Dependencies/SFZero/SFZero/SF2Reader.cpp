#include "SF2Reader.h"
#include "SF2Sound.h"
#include "SFZSample.h"
#include "RIFF.h"
#include "SF2.h"
#include "SF2Generator.h"
#include "SFZDebug.h"

using namespace SFZero;


SF2Reader::SF2Reader(SF2Sound* soundIn, const File& fileIn)
	: sound(soundIn)
{
	file = fileIn.createInputStream();
}


SF2Reader::~SF2Reader()
{
	delete file;
}


void SF2Reader::read()
{
	if (file == NULL) {
		sound->addError("Couldn't open file.");
		return;
		}

	// Read the hydra.
	SF2::Hydra hydra;
	file->setPosition(0);
	RIFFChunk riffChunk;
	riffChunk.ReadFrom(file);
	while (file->getPosition() < riffChunk.End()) {
		RIFFChunk chunk;
		chunk.ReadFrom(file);
		if (FourCCEquals(chunk.id, "pdta")) {
			hydra.ReadFrom(file, chunk.End());
			break;
			}
		chunk.SeekAfter(file);
		}
	if (!hydra.IsComplete()) {
		sound->addError("Invalid SF2 file (missing or incomplete hydra).");
		return;
		}

	// Read each preset.
	for (int whichPreset = 0; whichPreset < hydra.phdrNumItems - 1; ++whichPreset) {
		SF2::phdr* phdr = &hydra.phdrItems[whichPreset];
		SF2Sound::Preset* preset =
			new SF2Sound::Preset(phdr->presetName, phdr->bank, phdr->preset);
		sound->addPreset(preset);

		// Zones.
		//*** TODO: Handle global zone (modulators only).
		int presetZoneEnd = phdr[1].presetBagNdx;
		for (int presetWhichZone = phdr->presetBagNdx; presetWhichZone < presetZoneEnd; ++presetWhichZone) {
			SF2::pbag* pbag = &hydra.pbagItems[presetWhichZone];
			SFZRegion presetRegion;
			presetRegion.clearForRelativeSF2();

			// Generators.
			int presetGenEnd = pbag[1].genNdx;
			for (int presetWhichGen = pbag->genNdx; presetWhichGen < presetGenEnd; ++presetWhichGen) {
				SF2::pgen* pgen = &hydra.pgenItems[presetWhichGen];

				// Instrument.
				if (pgen->genOper == SF2Generator::instrument) {
					word whichInst = pgen->genAmount.wordAmount;
					if (whichInst < hydra.instNumItems) {
						SFZRegion instRegion;
						instRegion.clearForSF2();
						// Preset generators are supposed to be "relative" modifications of
						// the instrument settings, but that makes no sense for ranges.
						// For those, we'll have the instrument's generator take
						// precedence, though that may not be correct.
						instRegion.lokey = presetRegion.lokey;
						instRegion.hikey = presetRegion.hikey;
						instRegion.lovel = presetRegion.lovel;
						instRegion.hivel = presetRegion.hivel;

						SF2::inst* inst = &hydra.instItems[whichInst];
						int firstZone = inst->instBagNdx;
						int instZoneEnd = inst[1].instBagNdx;
						for (int instWhichZone = firstZone; instWhichZone < instZoneEnd; ++instWhichZone) {
							SF2::ibag* ibag = &hydra.ibagItems[instWhichZone];

							// Generators.
							SFZRegion zoneRegion = instRegion;
							bool hadSampleID = false;
							int instGenEnd = ibag[1].instGenNdx;
							for (int instWhichGen = ibag->instGenNdx; instWhichGen < instGenEnd; ++instWhichGen) {
								SF2::igen* igen = &hydra.igenItems[instWhichGen];
								if (igen->genOper == SF2Generator::sampleID) {
									int whichSample = igen->genAmount.wordAmount;
									SF2::shdr* shdr = &hydra.shdrItems[whichSample];
									zoneRegion.addForSF2(&presetRegion);
									zoneRegion.sf2ToSFZ();
									zoneRegion.offset += shdr->start;
									zoneRegion.end += shdr->end;
									zoneRegion.loop_start += shdr->startLoop;
									zoneRegion.loop_end += shdr->endLoop;
									if (shdr->endLoop > 0)
										zoneRegion.loop_end -= 1;
									if (zoneRegion.pitch_keycenter == -1)
										zoneRegion.pitch_keycenter = shdr->originalPitch;
									zoneRegion.tune += shdr->pitchCorrection;

									// Pin initialAttenuation to max +6dB.
									if (zoneRegion.volume > 6.0) {
										zoneRegion.volume = 6.0;
										sound->addUnsupportedOpcode(
											"extreme gain in initialAttenuation");
										}

									SFZRegion* newRegion = new SFZRegion();
									*newRegion = zoneRegion;
									newRegion->sample = sound->sampleFor(shdr->sampleRate);
									preset->addRegion(newRegion);
									hadSampleID = true;
									}
								else
									addGeneratorToRegion(igen->genOper, &igen->genAmount, &zoneRegion);
								}

							// Handle instrument's global zone.
							if (instWhichZone == firstZone && !hadSampleID)
								instRegion = zoneRegion;

							// Modulators.
							int modEnd = ibag[1].instModNdx;
							int whichMod = ibag->instModNdx;
							if (whichMod < modEnd)
								sound->addUnsupportedOpcode("any modulator");
							}
						}
					else
						sound->addError("Instrument out of range.");
					}

				// Other generators.
				else
					addGeneratorToRegion(pgen->genOper, &pgen->genAmount, &presetRegion);
				}

			// Modulators.
			int modEnd = pbag[1].modNdx;
			int whichMod = pbag->modNdx;
			if (whichMod < modEnd)
				sound->addUnsupportedOpcode("any modulator");
			}
		}
}


AudioSampleBuffer* SF2Reader::readSamples(
	double* progressVar, Thread* thread)
{
	static const unsigned long bufferSize = 32768;

	if (file == NULL) {
		sound->addError("Couldn't open file.");
		return NULL;
		}

	// Find the "sdta" chunk.
	file->setPosition(0);
	RIFFChunk riffChunk;
	riffChunk.ReadFrom(file);
	bool found = false;
	RIFFChunk chunk;
	while (file->getPosition() < riffChunk.End()) {
		chunk.ReadFrom(file);
		if (FourCCEquals(chunk.id, "sdta")) {
			found = true;
			break;
			}
		chunk.SeekAfter(file);
		}
	int64 sdtaEnd = chunk.End();
	found = false;
	while (file->getPosition() < sdtaEnd) {
		chunk.ReadFrom(file);
		if (FourCCEquals(chunk.id, "smpl")) {
			found = true;
			break;
			}
		chunk.SeekAfter(file);
		}
	if (!found) {
		sound->addError("SF2 is missing its \"smpl\" chunk.");
		return NULL;
		}

	// Allocate the AudioSampleBuffer.
	unsigned long numSamples = chunk.size / sizeof(short);
	AudioSampleBuffer* sampleBuffer = new AudioSampleBuffer(1, numSamples);

	// Read and convert.
	short* buffer = new short[bufferSize];
	unsigned long samplesLeft = numSamples;
	float* out = sampleBuffer->getWritePointer(0);
	while (samplesLeft > 0) {
		// Read the buffer.
		unsigned long samplesToRead = bufferSize;
		if (samplesToRead > samplesLeft)
			samplesToRead = samplesLeft;
		file->read(buffer, samplesToRead * sizeof(short));

		// Convert from signed 16-bit to float.
		unsigned long samplesToConvert = samplesToRead;
		short* in = buffer;
		for (; samplesToConvert > 0; --samplesToConvert) {
			// If we ever need to compile for big-endian platforms, we'll need to
			// byte-swap here.
			*out++ = *in++ / 32767.0f;
			}

		samplesLeft -= samplesToRead;

		if (progressVar)
			*progressVar = (float) (numSamples - samplesLeft) / numSamples;
		if (thread && thread->threadShouldExit()) {
			delete[] buffer;
			delete sampleBuffer;
			return NULL;
			}
		}
	delete[] buffer;

	if (progressVar)
		*progressVar = 1.0;

	return sampleBuffer;
}


void SF2Reader::addGeneratorToRegion(
	word genOper, SF2::genAmountType* amount, SFZRegion* region)
{
	switch (genOper) {
		case SF2Generator::startAddrsOffset:
			region->offset += amount->shortAmount;
			break;
		case SF2Generator::endAddrsOffset:
			region->end += amount->shortAmount;
			break;
		case SF2Generator::startloopAddrsOffset:
			region->loop_start += amount->shortAmount;
			break;
		case SF2Generator::endloopAddrsOffset:
			region->loop_end += amount->shortAmount;
			break;
		case SF2Generator::startAddrsCoarseOffset:
			region->offset += amount->shortAmount * 32768;
			break;
		case SF2Generator::modLfoToPitch:
			region->modLfoToPitch = amount->shortAmount;
			break;
		case SF2Generator::vibLfoToPitch:
			region->vibLfoToPitch = amount->shortAmount;
			break;
		case SF2Generator::modEnvToPitch:
			region->modEnvToPitch = amount->shortAmount;
			break;
		case SF2Generator::initialFilterFc:
			region->initialFilterFc = amount->shortAmount;
			break;
		case SF2Generator::initialFilterQ:
			region->initialFilterQ = amount->shortAmount;
			break;
		case SF2Generator::modLfoToFilterFc:
			region->modLfoToFilterFc = amount->shortAmount;
			break;
		case SF2Generator::modEnvToFilterFc:
			region->modEnvToFilterFc = amount->shortAmount;
			break;
		case SF2Generator::endAddrsCoarseOffset:
			region->end += amount->shortAmount * 32768;
			break;
		case SF2Generator::modLfoToVolume:
			region->modLfoToVolume = amount->shortAmount;
			break;
		case SF2Generator::pan:
			region->pan = amount->shortAmount * (2.0f / 10.0f);
			break;
		case SF2Generator::delayModLFO:
			region->delayModLFO = amount->shortAmount;
			break;
		case SF2Generator::freqModLFO:
			region->freqModLFO = amount->shortAmount;
			break;
		case SF2Generator::delayVibLFO:
			region->delayVibLFO = amount->shortAmount;
			break;
		case SF2Generator::freqVibLFO:
			region->freqVibLFO = amount->shortAmount;
			break;
		case SF2Generator::delayModEnv:
			region->modeg.delay = amount->shortAmount;
			break;
		case SF2Generator::attackModEnv:
			region->modeg.attack = amount->shortAmount;
			break;
		case SF2Generator::holdModEnv:
			region->modeg.hold = amount->shortAmount;
			break;
		case SF2Generator::decayModEnv:
			region->modeg.decay = amount->shortAmount;
			break;
		case SF2Generator::sustainModEnv:
			region->modeg.sustain = amount->shortAmount;
			break;
		case SF2Generator::releaseModEnv:
			region->modeg.release = amount->shortAmount;
			break;
		case SF2Generator::keynumToModEnvHold:
			region->modeg.keynumToHold = amount->shortAmount;
			break;
		case SF2Generator::keynumToModEnvDecay:
			region->modeg.keynumToDecay = amount->shortAmount;
			break;
		case SF2Generator::delayVolEnv:
			region->ampeg.delay = amount->shortAmount;
			break;
		case SF2Generator::attackVolEnv:
			region->ampeg.attack = amount->shortAmount;
			break;
		case SF2Generator::holdVolEnv:
			region->ampeg.hold = amount->shortAmount;
			break;
		case SF2Generator::decayVolEnv:
			region->ampeg.decay = amount->shortAmount;
			break;
		case SF2Generator::sustainVolEnv:
			region->ampeg.sustain = amount->shortAmount;
			break;
		case SF2Generator::releaseVolEnv:
			region->ampeg.release = amount->shortAmount;
			break;
		case SF2Generator::keynumToVolEnvHold:
			region->ampeg.keynumToHold = amount->shortAmount;
			break;
		case SF2Generator::keynumToVolEnvDecay:
			region->ampeg.keynumToDecay = amount->shortAmount;
			break;
		case SF2Generator::keyRange:
			region->lokey = amount->range.lo;
			region->hikey = amount->range.hi;
			break;
		case SF2Generator::velRange:
			region->lovel = amount->range.lo;
			region->hivel = amount->range.hi;
			break;
		case SF2Generator::startloopAddrsCoarseOffset:
			region->loop_start += amount->shortAmount * 32768;
			break;
		case SF2Generator::initialAttenuation:
			// The spec says "initialAttenuation" is in centibels.  But everyone
			// seems to treat it as millibels.
			region->volume += -amount->shortAmount / 100.0f;
			break;
		case SF2Generator::endloopAddrsCoarseOffset:
			region->loop_end += amount->shortAmount * 32768;
			break;
		case SF2Generator::coarseTune:
			region->transpose += amount->shortAmount;
			break;
		case SF2Generator::fineTune:
			region->tune += amount->shortAmount;
			break;
		case SF2Generator::sampleModes:
			{
				SFZRegion::LoopMode loopModes[] = {
					SFZRegion::no_loop, SFZRegion::loop_continuous,
					SFZRegion::no_loop, SFZRegion::loop_sustain };
				region->loop_mode = loopModes[amount->wordAmount & 0x03];
			}
			break;
		case SF2Generator::scaleTuning:
			region->pitch_keytrack = amount->shortAmount;
			break;
		case SF2Generator::exclusiveClass:
			region->group = region->off_by = amount->wordAmount;
			break;
		case SF2Generator::overridingRootKey:
			region->pitch_keycenter = amount->shortAmount;
			break;
		case SF2Generator::endOper:
			// Ignore.
			break;

		case SF2Generator::unused1:
		case SF2Generator::chorusEffectsSend:
		case SF2Generator::reverbEffectsSend:
		case SF2Generator::unused2:
		case SF2Generator::unused3:
		case SF2Generator::unused4:
		case SF2Generator::instrument:
			// Only allowed in certain places, where we already special-case it.
		case SF2Generator::reserved1:
		case SF2Generator::keynum:
		case SF2Generator::velocity:
		case SF2Generator::reserved2:
		case SF2Generator::sampleID:
			// Only allowed in certain places, where we already special-case it.
		case SF2Generator::reserved3:
		case SF2Generator::unused5:
		default:
			{
				const SF2Generator* generator = GeneratorFor(genOper);
				sound->addUnsupportedOpcode(generator->name);
			}
			break;
		}
}



