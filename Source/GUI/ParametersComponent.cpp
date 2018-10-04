/*
  ==============================================================================

    ParametersComponent.cpp
    Created: 3 Oct 2018 6:52:53pm
    Author:  COx2

  ==============================================================================
*/

#include "ParametersComponent.h"

// ①当ソースファイル内で共通して使用する定数宣言を無名名前空間にまとめておく。
namespace {
	const Colour PANEL_COLOUR = juce::Colour(36, 36, 36);			// パネル背景を塗りつぶす色(暗めのグレー)
	const float PANEL_NAME_FONT_SIZE = 24.0f;						// パネル名を表示する文字の大きさ
	const float PARAM_LABEL_FONT_SIZE = 16.0f;						// スライダー名を表示する文字の大きさ
	const int PANEL_NAME_HEIGHT = 42;								// パネル名を表示する領域のY方向大きさ
	const int LOCAL_MARGIN = 2;										// パネルの周囲に確保する間隔の大きさ
}


AmpEGParametersComponent::AmpEGParametersComponent(AmpEGParameters * ampEnvParams)
	:_ampEnvParamsPtr(ampEnvParams)
	, attackSlider(Slider::SliderStyle::RotaryHorizontalVerticalDrag, Slider::TextEntryBoxPosition::NoTextBox)
	, decaySlider(Slider::SliderStyle::RotaryHorizontalVerticalDrag, Slider::TextEntryBoxPosition::NoTextBox)
	, sustainSlider(Slider::SliderStyle::RotaryHorizontalVerticalDrag, Slider::TextEntryBoxPosition::NoTextBox)
	, releaseSlider(Slider::SliderStyle::RotaryHorizontalVerticalDrag, Slider::TextEntryBoxPosition::NoTextBox)
{
	Font paramLabelFont = Font(PARAM_LABEL_FONT_SIZE, Font::plain).withTypefaceStyle("Regular");

	attackSlider.setRange(_ampEnvParamsPtr->Attack->range.start, _ampEnvParamsPtr->Attack->range.end, 0.01);
	attackSlider.setValue(_ampEnvParamsPtr->Attack->get(), dontSendNotification);
	attackSlider.setPopupDisplayEnabled(true, true, this);
	attackSlider.setPopupMenuEnabled(true);
	attackSlider.setTextValueSuffix(" seconds");
	attackSlider.addListener(this);
	addAndMakeVisible(attackSlider);

	decaySlider.setRange(_ampEnvParamsPtr->Decay->range.start, _ampEnvParamsPtr->Decay->range.end, 0.01);
	decaySlider.setValue(_ampEnvParamsPtr->Decay->get(), dontSendNotification);
	decaySlider.setPopupDisplayEnabled(true, true, this);
	decaySlider.setPopupMenuEnabled(true);
	decaySlider.setTextValueSuffix(" seconds");
	decaySlider.addListener(this);
	addAndMakeVisible(decaySlider);

	sustainSlider.setRange(_ampEnvParamsPtr->Sustain->range.start, _ampEnvParamsPtr->Sustain->range.end, 0.01);
	sustainSlider.setValue(_ampEnvParamsPtr->Sustain->get(), dontSendNotification);
	sustainSlider.setPopupDisplayEnabled(true, true, this);
	sustainSlider.setPopupMenuEnabled(true);
	sustainSlider.addListener(this);
	addAndMakeVisible(sustainSlider);

	releaseSlider.setRange(_ampEnvParamsPtr->Release->range.start, _ampEnvParamsPtr->Release->range.end, 0.01);
	releaseSlider.setValue(_ampEnvParamsPtr->Release->get(), dontSendNotification);
	releaseSlider.setPopupDisplayEnabled(true, true, this);
	releaseSlider.setPopupMenuEnabled(true);
	releaseSlider.setTextValueSuffix(" seconds");
	releaseSlider.addListener(this);
	addAndMakeVisible(releaseSlider);

	attackLabel.setFont(paramLabelFont);
	attackLabel.setText("Attack", dontSendNotification);
	attackLabel.setJustificationType(Justification::centred);
	attackLabel.setEditable(false, false, false);
	addAndMakeVisible(attackLabel);

	decayLabel.setFont(paramLabelFont);
	decayLabel.setText("Decay", dontSendNotification);
	decayLabel.setJustificationType(Justification::centred);
	decayLabel.setEditable(false, false, false);
	addAndMakeVisible(decayLabel);

	sustainLabel.setFont(paramLabelFont);
	sustainLabel.setText("Sustain", dontSendNotification);
	sustainLabel.setJustificationType(Justification::centred);
	sustainLabel.setEditable(false, false, false);
	addAndMakeVisible(sustainLabel);

	releaseLabel.setFont(paramLabelFont);
	releaseLabel.setText("Release", dontSendNotification);
	releaseLabel.setJustificationType(Justification::centred);
	releaseLabel.setEditable(false, false, false);
	addAndMakeVisible(releaseLabel);

	startTimerHz(30.0f);
}

AmpEGParametersComponent::~AmpEGParametersComponent()
{}

void AmpEGParametersComponent::paint(Graphics & g)
{
	Font panelNameFont = Font(PANEL_NAME_FONT_SIZE, Font::plain).withTypefaceStyle("Italic");
	{
		float x = 0.0f, y = 0.0f, width = (float)getWidth(), height = (float)getHeight();
		g.setColour(PANEL_COLOUR);
		g.fillRoundedRectangle(x, y, width, height, 10.0f);
	}
	{
		Rectangle<int> bounds = getLocalBounds();
		Rectangle<int> textArea = bounds.removeFromTop(PANEL_NAME_HEIGHT).reduced(LOCAL_MARGIN);

		String text("AMP EG");
		Colour textColour = Colours::white;
		g.setColour(textColour);
		g.setFont(panelNameFont);
		g.drawText(text, textArea, Justification::centred, false);
	}
}

void AmpEGParametersComponent::resized()
{
	float rowSize = 4.0f;
	float divide = 1.0f / rowSize;
	int labelHeight = 20;

	Rectangle<int> bounds = getLocalBounds(); // コンポーネント基準の値
	bounds.removeFromTop(PANEL_NAME_HEIGHT);
	{
		Rectangle<int> area = bounds.removeFromLeft(getWidth() * divide);
		attackLabel.setBounds(area.removeFromTop(labelHeight).reduced(LOCAL_MARGIN));
		attackSlider.setBounds(area.reduced(LOCAL_MARGIN));
	}
	{
		Rectangle<int> area = bounds.removeFromLeft(getWidth() * divide);
		decayLabel.setBounds(area.removeFromTop(labelHeight).reduced(LOCAL_MARGIN));
		decaySlider.setBounds(area.reduced(LOCAL_MARGIN));
	}
	{
		Rectangle<int> area = bounds.removeFromLeft(getWidth() * divide);
		sustainLabel.setBounds(area.removeFromTop(labelHeight).reduced(LOCAL_MARGIN));
		sustainSlider.setBounds(area.reduced(LOCAL_MARGIN));
	}
	{
		Rectangle<int> area = bounds.removeFromLeft(getWidth() * divide);
		releaseLabel.setBounds(area.removeFromTop(labelHeight).reduced(LOCAL_MARGIN));
		releaseSlider.setBounds(area.reduced(LOCAL_MARGIN));
	}
}

void AmpEGParametersComponent::timerCallback()
{
	attackSlider.setValue(_ampEnvParamsPtr->Attack->get(), dontSendNotification);
	decaySlider.setValue(_ampEnvParamsPtr->Decay->get(), dontSendNotification);
	sustainSlider.setValue(_ampEnvParamsPtr->Sustain->get(), dontSendNotification);
	releaseSlider.setValue(_ampEnvParamsPtr->Release->get(), dontSendNotification);
}

void AmpEGParametersComponent::sliderValueChanged(Slider * slider)
{
	if (slider == &attackSlider)
	{
		*_ampEnvParamsPtr->Attack = (float)attackSlider.getValue();
	}
	else if (slider == &decaySlider)
	{
		*_ampEnvParamsPtr->Decay = (float)decaySlider.getValue();
	}
	else if (slider == &sustainSlider)
	{
		*_ampEnvParamsPtr->Sustain = (float)sustainSlider.getValue();
	}
	else if (slider == &releaseSlider)
	{
		*_ampEnvParamsPtr->Release = (float)releaseSlider.getValue();
	}
}