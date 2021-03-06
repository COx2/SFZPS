﻿/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

#include "GUI/ScopeComponent.h"
#include "GUI/ParametersComponent.h"

//==============================================================================
/**
*/
class SfzpsAudioProcessorEditor  : public AudioProcessorEditor, Button::Listener, Slider::Listener, private Timer
{
public:
    SfzpsAudioProcessorEditor (SfzpsAudioProcessor&);
    ~SfzpsAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

	virtual void buttonClicked(Button* buttonClicked) override;
	virtual void sliderValueChanged(Slider* sliderChanged) override;
	virtual void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SfzpsAudioProcessor& processor;

	MidiKeyboardComponent keyboardComponent;

	ScopeComponent<float> scopeComponent;

	TextButton loadSoundFontButton;

	Label currentSoundFontName;

	Label currentSubSoundLabel;
	Slider subSoundSelectorSlider;

	AmpEGParametersComponent ampEGParamComponents;

	LookAndFeel* customLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfzpsAudioProcessorEditor)
};
