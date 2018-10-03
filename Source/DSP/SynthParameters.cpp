/*
  ==============================================================================

    SynthParameters.cpp
    Created: 3 Oct 2018 4:34:13pm
    Author:  COx2

  ==============================================================================
*/

#include "SynthParameters.h"

AmpEGParameters::AmpEGParameters(AudioParameterFloat* attack,
	AudioParameterFloat* decay,
	AudioParameterFloat* sustain,
	AudioParameterFloat* release)
	: Attack(attack)
	, Decay(decay)
	, Sustain(sustain)
	, Release(release)
{}

void AmpEGParameters::addAllParameters(AudioProcessor& processor)
{
	processor.addParameter(Attack);
	processor.addParameter(Decay);
	processor.addParameter(Sustain);
	processor.addParameter(Release);
}

void AmpEGParameters::saveParameters(XmlElement & xml)
{
	xml.setAttribute(Attack->paramID, (double)Attack->get());
	xml.setAttribute(Decay->paramID, (double)Decay->get());
	xml.setAttribute(Sustain->paramID, (double)Sustain->get());
	xml.setAttribute(Release->paramID, (double)Release->get());
}

void AmpEGParameters::loadParameters(XmlElement & xml)
{
	*Attack = (float)xml.getDoubleAttribute(Attack->paramID, 0.01);
	*Decay = (float)xml.getDoubleAttribute(Decay->paramID, 0.01);
	*Sustain = (float)xml.getDoubleAttribute(Sustain->paramID, 1.0);
	*Release = (float)xml.getDoubleAttribute(Release->paramID, 0.01);
}
