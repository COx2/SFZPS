#ifndef SFZVoice_h
#define SFZVoice_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "SFZEG.h"

class AmpEGParameters;

namespace SFZero {

class SFZRegion;


class JUCE_API SFZVoice : public juce::SynthesiserVoice 
{
public:
	SFZVoice();
	SFZVoice(AmpEGParameters* ampEGParams);

	~SFZVoice();

    bool	canPlaySound(juce::SynthesiserSound* sound);

    void	startNote(const int midiNoteNumber,	const float velocity, juce::SynthesiserSound* sound, const int currentPitchWheelPosition);
    
	void	stopNote(float velocity, const bool allowTailOff);
	void	stopNoteForGroup();
	void	stopNoteQuick();

    void	pitchWheelMoved(const int newValue);

    void	controllerMoved(const int controllerNumber,	const int newValue);
	void	renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples);
	
	bool	isPlayingNoteDown();
	bool	isPlayingOneShot();

	int	getGroup();
	int	getOffBy();

	// Set the region to be used by the next startNote().
	void	setRegion(SFZRegion* nextRegion);

	juce::String	infoString();

protected:
	SFZRegion*	region;
	int       	curMidiNote, curPitchWheel;
	double    	pitchInputTimecents, pitchOutputFactor;
	double    	sourceSamplePosition;
	float     	noteGainDB, panFactorLeft, panFactorRight;
	SFZEG     	ampeg, modeg;
	unsigned long	sampleEnd;
	unsigned long	loopStart, loopEnd;

	// Info only.
	unsigned long	numLoops;
	int	curVelocity;

	void	calcPitchRatio();
	void	killNote();

	// Low-pass filter.
	struct SFZLowpass {
		void setup(float Fc);
		inline float process(double In, double& z1, double& z2);
		double QInv, a0, a1, b1, b2;
		double Z1Left, Z2Left, Z1Right, Z2Right;
		bool active;
	} lowpass;

	// Low Frequency Oscillators.
	struct SFZLFO {
		void setup(float delay, int freqCents, float sampleRate);
		inline void process(int blockSamples);
		long   samplesUntil;
		float  level, delta;
	} modlfo, viblfo;


private:
	// MOD: AMP EG
	AmpEGParameters* _ampEGParamsPtr;
};

}

#endif 	// SFZVoice_h
