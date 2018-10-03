#ifndef SF2Sound_h
#define SF2Sound_h

#include "SFZSound.h"


namespace SFZero {

class SF2Sound : public SFZSound {
	public:
    SF2Sound(const juce::File& file);
		~SF2Sound();

		void	loadRegions();
		void	loadSamples(
                            juce::AudioFormatManager* formatManager,
                            double* progressVar = NULL, juce::Thread* thread = NULL);

		struct Preset {
            juce::String	name;
			int    	bank;
			int   	preset;
            juce::OwnedArray<SFZRegion>	regions;

            Preset(juce::String nameIn, int bankIn, int presetIn)
				: name(nameIn), bank(bankIn), preset(presetIn) {}
			~Preset() {}

			void	addRegion(SFZRegion* region) {
				regions.add(region);
				}
			};
		void	addPreset(Preset* preset);

		int	numSubsounds();
        juce::String    subsoundName(int whichSubsound);
		void	useSubsound(int whichSubsound);
		int 	selectedSubsound();

		SFZSample*	sampleFor(unsigned long sampleRate);
        void    setSamplesBuffer(juce::AudioSampleBuffer* buffer);

	protected:
        juce::OwnedArray<Preset>    presets;
        juce::HashMap<juce::int64, SFZSample*>    samplesByRate;
		int               	selectedPreset;
	};

}

#endif 	// !SF2Sound_h

