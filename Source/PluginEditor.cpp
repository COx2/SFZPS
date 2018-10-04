/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
	const int KEY_WIDTH = 32.0f;
	const int KEY_HEIGHT = 80.0f;
	const int PANEL_MARGIN = 2;
}

//==============================================================================
SfzpsAudioProcessorEditor::SfzpsAudioProcessorEditor(SfzpsAudioProcessor& p)
	: AudioProcessorEditor(&p), processor(p)
	, keyboardComponent(p.getKeyboardState(), MidiKeyboardComponent::Orientation::horizontalKeyboard)
	, scopeComponent(p.getScopeDataQueue())
	, ampEGParamComponents(&p.ampEGParameters)
	, subSoundSelectorSlider(Slider::SliderStyle::IncDecButtons, Slider::TextEntryBoxPosition::TextBoxLeft)
	, currentSubSoundLabel("SUB_SOUND_NO", "SubSound No: ")
{

	keyboardComponent.setKeyWidth(KEY_WIDTH);
	addAndMakeVisible(keyboardComponent);

	loadSoundFontButton.setButtonText("Load SFZ/SF2 File...");
	loadSoundFontButton.addListener(this);
	addAndMakeVisible(loadSoundFontButton);

	currentSoundFontName.setText("Soundfont Name", dontSendNotification);
	addAndMakeVisible(currentSoundFontName);

	addAndMakeVisible(ampEGParamComponents);

	subSoundSelectorSlider.setRange(0, 255, 1);
	subSoundSelectorSlider.addListener(this);
	addAndMakeVisible(subSoundSelectorSlider);

	addAndMakeVisible(currentSubSoundLabel);
	
	addAndMakeVisible(scopeComponent);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (640, 480);

	startTimerHz(30);
}

SfzpsAudioProcessorEditor::~SfzpsAudioProcessorEditor()
{
}

//==============================================================================
void SfzpsAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

	currentSoundFontName.setText(processor.getCurrentFileName(), dontSendNotification);
}

void SfzpsAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

	Rectangle<int> bounds = getLocalBounds();

	keyboardComponent.setBounds(bounds.removeFromBottom(KEY_HEIGHT).reduced(PANEL_MARGIN));

	scopeComponent.setBounds(bounds.removeFromBottom(200).reduced(PANEL_MARGIN));

	auto leftArea = bounds.removeFromLeft(160);
	{
		loadSoundFontButton.setBounds(leftArea.removeFromTop(60).reduced(PANEL_MARGIN));
		currentSoundFontName.setBounds(leftArea.removeFromTop(60).reduced(PANEL_MARGIN));
		
		currentSubSoundLabel.setBounds(leftArea.removeFromTop(20).reduced(PANEL_MARGIN));
		subSoundSelectorSlider.setBounds(leftArea.reduced(PANEL_MARGIN));
	}

	ampEGParamComponents.setBounds(bounds);
}

void SfzpsAudioProcessorEditor::buttonClicked(Button* buttonClicked)
{
	if (buttonClicked == &loadSoundFontButton)
	{
		FileChooser chooser("Select a SFZ/SF2 file to play...", File::nonexistent, "*.sfz; *.SFZ; *.sf2; *.SF2");

		if (chooser.browseForFileToOpen())
		{
			File file(chooser.getResult());

			processor.setSoundFontSample(file.getFileName(), &file);
		}
		else
		{
		}
	}
}

void SfzpsAudioProcessorEditor::sliderValueChanged(Slider* sliderChanged)
{
	if (sliderChanged == &subSoundSelectorSlider)
	{
		if(subSoundSelectorSlider.getValue() < processor.subSoundNum)
			*processor.subSoundSelector = subSoundSelectorSlider.getValue();
	}
}

void SfzpsAudioProcessorEditor::timerCallback()
{
	subSoundSelectorSlider.setValue(processor.subSoundSelector->get(), dontSendNotification);
}
