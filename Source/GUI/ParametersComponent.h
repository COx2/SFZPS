/*
  ==============================================================================

    ParametersComponent.h
    Created: 3 Oct 2018 6:52:53pm
    Author:  COx2

  ==============================================================================
*/

#pragma once

#include "../DSP/SynthParameters.h"

class AmpEGParametersComponent : public Component, Slider::Listener, private Timer
{
public:
	AmpEGParametersComponent(AmpEGParameters* ampEnvParams);
	virtual ~AmpEGParametersComponent();

	virtual void paint(Graphics& g) override;
	virtual void resized() override;

private:
	AmpEGParametersComponent();

	virtual void timerCallback() override;
	virtual void sliderValueChanged(Slider* slider) override;

	AmpEGParameters* _ampEnvParamsPtr;

	Slider attackSlider;
	Slider decaySlider;
	Slider sustainSlider;
	Slider releaseSlider;

	Label attackLabel;
	Label decayLabel;
	Label sustainLabel;
	Label releaseLabel;
};