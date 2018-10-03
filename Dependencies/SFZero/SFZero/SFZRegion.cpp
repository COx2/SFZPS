#include "SFZRegion.h"
#include "SFZSample.h"
#include "SFZDebug.h"
#include <string.h>
#include <stdio.h>

using namespace SFZero;


void SFZEGParameters::clear()
{
	delay = 0.0f;
	start = 0.0f;
	attack = 0.0f;
	hold = 0.0f;
	decay = 0.0f;
	sustain = 100.0f;
	release = 0.0f;
	keynumToHold = 0.0f;
	keynumToDecay = 0.0f;
}


void SFZEGParameters::clearMod()
{
	// Clear for velocity or other modification.
	delay = start = attack = hold = decay = sustain = release = 0.0f;
}


void SFZEGParameters::sf2ToSFZ(bool sustainIsGain)
{
	// EG times need to be converted from timecents to seconds.
	// Pin very short EG segments.  Timecents don't get to zero, and our EG is
	// happier with zero values.
	delay   = (delay   < -11950.0f ? 0.0f : SFZRegion::timecents2Secs(delay));
	attack  = (attack  < -11950.0f ? 0.0f : SFZRegion::timecents2Secs(attack));
	release = (release < -11950.0f ? 0.0f : SFZRegion::timecents2Secs(release));

	// If we have dynamic hold or decay times depending on key number we need
	// to keep the values in timecents so we can calculate it during startNote
	if (!keynumToHold)  hold  = (hold  < -11950.0f ? 0.0f : SFZRegion::timecents2Secs(hold));
	if (!keynumToDecay) decay = (decay < -11950.0f ? 0.0f : SFZRegion::timecents2Secs(decay));
	
	if (sustain < 0.0f) sustain = 0.0f;
	else if (sustainIsGain) sustain = 100.0f * Decibels::decibelsToGain(-sustain / 10.0f);
	else sustain = sustain / 10.0f;
}


SFZRegion::SFZRegion()
{
	clear();
}


void SFZRegion::clear()
{
	memset(this, 0, sizeof(*this));
	hikey = 127;
	hivel = 127;
	pitch_keycenter = 60; 	// C4
	pitch_keytrack = 100;
	bend_up = 200;
	bend_down = -200;
	amp_veltrack = 100.0f;
	ampeg.clear();
	ampeg_veltrack.clearMod();
	modeg.clear();
	initialFilterFc = 0x7FFF;
	seq_length = 1;
	seq_position = 1;
	seq_count = 1; // seq_count counts from 1 <= seq_length
}


void SFZRegion::clearForSF2()
{
	clear();
	pitch_keycenter = -1;
	loop_mode = no_loop;

	// SF2 defaults in timecents.
	ampeg.delay = -12000.0f;
	ampeg.attack = -12000.0f;
	ampeg.hold = -12000.0f;
	ampeg.decay = -12000.0f;
	ampeg.sustain = 0.0f;
	ampeg.release = -12000.0f;

	modeg.delay = -12000.0f;
	modeg.attack = -12000.0f;
	modeg.hold = -12000.0f;
	modeg.decay = -12000.0f;
	modeg.sustain = 0.0f;
	modeg.release = -12000.0f;

	initialFilterFc = 13500;

	delayModLFO = -12000.0f;
	delayVibLFO = -12000.0f;
}


void SFZRegion::clearForRelativeSF2()
{
	clear();
	pitch_keytrack = 0;
	amp_veltrack = 0.0f;
	ampeg.sustain = 0.0f;
	modeg.sustain = 0.0f;
}


void SFZRegion::addForSF2(SFZRegion* other)
{
	offset += other->offset;
	end += other->end;
	loop_start += other->loop_start;
	loop_end += other->loop_end;
	transpose += other->transpose;
	tune += other->tune;
	pitch_keytrack += other->pitch_keytrack;
	volume += other->volume;
	pan += other->pan;

	ampeg.delay += other->ampeg.delay;
	ampeg.attack += other->ampeg.attack;
	ampeg.hold += other->ampeg.hold;
	ampeg.decay += other->ampeg.decay;
	ampeg.sustain += other->ampeg.sustain;
	ampeg.release += other->ampeg.release;

	modeg.delay += other->modeg.delay;
	modeg.attack += other->modeg.attack;
	modeg.hold += other->modeg.hold;
	modeg.decay += other->modeg.decay;
	modeg.sustain += other->modeg.sustain;
	modeg.release += other->modeg.release;

	initialFilterQ += other->initialFilterQ;
	initialFilterFc += other->initialFilterFc;
	modEnvToPitch += other->modEnvToPitch;
	modEnvToFilterFc += other->modEnvToFilterFc;
	delayModLFO += other->delayModLFO;
	freqModLFO += other->freqModLFO;
	modLfoToPitch += other->modLfoToPitch;
	modLfoToFilterFc += other->modLfoToFilterFc;
	modLfoToVolume += other->modLfoToVolume;
	delayVibLFO += other->delayVibLFO;
	freqVibLFO += other->freqVibLFO;
	vibLfoToPitch += other->vibLfoToPitch;
}


void SFZRegion::sf2ToSFZ()
{
	// EG times need to be converted from timecents to seconds.
	ampeg.sf2ToSFZ(true);
	modeg.sf2ToSFZ(false);
	
	// LFO times need to be converted from timecents to seconds.
	delayModLFO = (delayModLFO < -11950.0f ? 0.0f : timecents2Secs(delayModLFO));
	delayVibLFO = (delayVibLFO < -11950.0f ? 0.0f : timecents2Secs(delayVibLFO));

	// Pin values to their ranges.
	if (pan < -100.0f)
		pan = -100.0f;
	else if (pan > 100.0f)
		pan = 100.0f;

	if (initialFilterQ < 1500 || initialFilterQ > 13500)
		initialFilterQ = 0;
}

void SFZRegion::incrementSeqCount()
{
	seq_count = (seq_count % seq_length) + 1;
}

void SFZRegion::dump()
{
	printf("%d - %d, vel %d - %d", lokey, hikey, lovel, hivel);
	if (sample) {
		char name[64];
		sample->getShortName().copyToUTF8(name, 64);
		printf(": %s", name);
		}
	printf("\n");
}



