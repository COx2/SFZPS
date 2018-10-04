/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SfzpsAudioProcessor::SfzpsAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
	, scopeDataCollector(scopeDataQueue)
	// AMP EGモジュールを操作するためのパラメータ群
	, ampEGParameters{
		new AudioParameterFloat("AMPEG_ATTACK", "Attack", 0.001f, 3.0f, 0.001f),
		new AudioParameterFloat("AMPEG_DECAY", "Decay",  0.001f, 3.0f, 0.001f),
		new AudioParameterFloat("AMPEG_SUSTAIN", "Sustain", 0.0f, 100.0f, 100.0f),
		new AudioParameterFloat("AMPEG_RELEASE", "Release", 0.001f, 3.0f, 0.1f)
	}
	, subSoundSelector(new AudioParameterInt("SUBSOUND_SELECTOR","SubSound", 0, 255, 0))
	, currentFileName("File Name")
	, currentSampleFilePath("")
{

	ampEGParameters.addAllParameters(*this);
	addParameter(subSoundSelector);

}

SfzpsAudioProcessor::~SfzpsAudioProcessor()
{
	sfzSynth.clearVoices();
	sfzSynth.clearSounds();
}

//==============================================================================
const String SfzpsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SfzpsAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SfzpsAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SfzpsAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SfzpsAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SfzpsAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SfzpsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SfzpsAudioProcessor::setCurrentProgram (int index)
{
}

const String SfzpsAudioProcessor::getProgramName (int index)
{
    return {};
}

void SfzpsAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void SfzpsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

	sfzSynth.setCurrentPlaybackSampleRate(sampleRate);
}

void SfzpsAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SfzpsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SfzpsAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
	if(sfzSynth.getSound() != nullptr)
		sfzSynth.getSound()->useSubsound(subSoundSelector->get());

	// Merge MIDI messages.
	keyboardState.processNextMidiBuffer(midiMessages, 0 , buffer.getNumSamples(), true);

    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

    }

	sfzSynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

	scopeDataCollector.process(buffer.getReadPointer(0), (size_t)buffer.getNumSamples());
}

//==============================================================================
bool SfzpsAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* SfzpsAudioProcessor::createEditor()
{
    return new SfzpsAudioProcessorEditor (*this);
}

//==============================================================================
void SfzpsAudioProcessor::getStateInformation (MemoryBlock& destData)
{
	std::unique_ptr<XmlElement> xml(new XmlElement("SFZPSParameters"));
	
	ampEGParameters.saveParameters(*xml);
	
	xml->setAttribute("FilePath", currentSampleFilePath);

	xml->setAttribute(subSoundSelector->paramID, subSoundSelector->get());

	copyXmlToBinary(*xml, destData);
}

void SfzpsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
	if (xmlState.get() != nullptr)
	{
		if (xmlState->hasTagName("SFZPSParameters"))
		{
			ampEGParameters.loadParameters(*xmlState);

			currentSampleFilePath = xmlState->getStringAttribute("FilePath");
			if (currentSampleFilePath != "")
			{
				loadSampleFile(currentSampleFilePath);
				*subSoundSelector = xmlState->getIntAttribute(subSoundSelector->paramID, 0);
			}

		}
	}


}

bool SfzpsAudioProcessor::setSoundFontSample(String fileName, File* sfzFile)
{
	sfzSynth.clearVoices();
	sfzSynth.clearSounds();

	for (int i = 0; i < 128; i++)
	{
		sfzSynth.addVoice(new SFZero::SFZVoice(&ampEGParameters));
	}

	AudioFormatManager formatManager;
	formatManager.registerBasicFormats();
	auto extension = sfzFile->getFileExtension().toUpperCase();
	if (extension == ".SF2")
	{
		SFZero::SF2Sound* sound = new SFZero::SF2Sound(*sfzFile);
		sound->loadRegions();
		sound->loadSamples(&formatManager);

		subSoundNum = sound->numSubsounds();
		// デシリアライズして代入しないとヌルポになるから注意
		*subSoundSelector = 0;
		sound->useSubsound(subSoundSelector->get());

		sfzSynth.addSound(sound);
	}
	else if (extension == ".SFZ")
	{
		SFZero::SFZSound* sound = new SFZero::SFZSound(*sfzFile);
		sound->loadRegions();
		sound->loadSamples(&formatManager);

		// デシリアライズして代入しないとヌルポになるから注意
		*subSoundSelector = 0;
		subSoundNum = sound->numSubsounds();

		sfzSynth.addSound(sound);
	}
	else 
	{
		return false;
	}

#if DEBUG
	sfzSynth.getSound()->dump();
#endif

	currentFileName = fileName;

	currentSampleFilePath = sfzFile->getFullPathName();

	return true;
}

bool SfzpsAudioProcessor::loadSampleFile(String filePath)
{
	File file(filePath);

	if (!file.existsAsFile())
	{
		return false;
	}
	
	setSoundFontSample(file.getFileName(), &file);

	return true;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SfzpsAudioProcessor();
}
