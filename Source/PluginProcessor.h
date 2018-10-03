/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "GUI/ScopeComponent.h"
#include "DSP/SynthParameters.h"

//==============================================================================
/**
*/
class SfzpsAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    SfzpsAudioProcessor();
    ~SfzpsAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;



	//==============================================================================
	MidiKeyboardState& getKeyboardState() { return keyboardState; }
	AudioBufferQueue<float>& getScopeDataQueue() { return scopeDataQueue; }
	juce::String getCurrentFileName() { return currentFileName; }


	bool setSoundFontSample(String fileName, File* sfzFile);

	AmpEGParameters ampEGParameters;
	AudioParameterInt* subSoundSelector;


private:
	//Synthesiser synth;
	SFZero::SFZSynth sfzSynth;

	juce::String currentFileName;

	MidiKeyboardState keyboardState;

	AudioBufferQueue<float> scopeDataQueue;
	ScopeDataCollector<float> scopeDataCollector;
	 
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfzpsAudioProcessor)
};
