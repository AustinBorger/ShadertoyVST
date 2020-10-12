/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ShadertoyAudioProcessor::ShadertoyAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    programParam = std::move(std::unique_ptr<juce::AudioParameterInt>(new juce::AudioParameterInt("program", "program", 0, 100, 0)));
    addParameter(programParam.get());

    for (int i = 0; i < 256; i++) {
        addUniformFloat("float" + std::to_string(i));
    }
    
    for (int i = 0; i < 256; i++) {
        addUniformInt("int" + std::to_string(i));
    }
}

ShadertoyAudioProcessor::~ShadertoyAudioProcessor()
{
}

//==============================================================================
const juce::String ShadertoyAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ShadertoyAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ShadertoyAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ShadertoyAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ShadertoyAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ShadertoyAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ShadertoyAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ShadertoyAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ShadertoyAudioProcessor::getProgramName (int index)
{
    return {};
}

void ShadertoyAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ShadertoyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void ShadertoyAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ShadertoyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
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

void ShadertoyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool ShadertoyAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ShadertoyAudioProcessor::createEditor()
{
    return new ShadertoyAudioProcessorEditor (*this);
}

//==============================================================================
void ShadertoyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ShadertoyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

void ShadertoyAudioProcessor::addUniformFloat(const juce::String &name)
{
    juce::NormalisableRange<float> normalisableRange(0.0f, 1.0f, 0.0f);
    juce::String paramId = "float" + std::to_string(floatParams.size());
    floatParams.emplace_back(std::move(std::unique_ptr<juce::AudioParameterFloat>
        (new juce::AudioParameterFloat(paramId, name, normalisableRange, 0.0f))));
    addParameter(floatParams.back().get());
}

void ShadertoyAudioProcessor::addUniformInt(const juce::String &name)
{
    juce::String paramId = "int" + std::to_string(intParams.size());
    intParams.emplace_back(std::move(std::unique_ptr<juce::AudioParameterInt>
        (new juce::AudioParameterInt(paramId, name, 0, 100, 0))));
    addParameter(intParams.back().get());
}

float ShadertoyAudioProcessor::getUniformFloat(int i)
{
    return floatParams[i]->get();
}

int ShadertoyAudioProcessor::getUniformInt(int i)
{
    return intParams[i]->get();
}

int ShadertoyAudioProcessor::getProgramIdx()
{
    return programParam->get();
}

void ShadertoyAudioProcessor::addShaderFileEntry()
{
    shaderFiles.emplace_back();
    shaderStrings.emplace_back();
}

void ShadertoyAudioProcessor::removeShaderFileEntry(int idx)
{
    shaderFiles.erase(shaderFiles.begin() + idx);
    shaderStrings.erase(shaderStrings.begin() + idx);
}

void ShadertoyAudioProcessor::setShaderFile(int idx, juce::String shaderFile)
{
    shaderFiles[idx] = std::move(shaderFile);
    juce::File file(shaderFiles[idx]);
    shaderStrings[idx] = file.loadFileAsString();
}

const juce::String &ShadertoyAudioProcessor::getShaderFile(int idx)
{
    return shaderFiles[idx];
}

const juce::String &ShadertoyAudioProcessor::getShaderString(int idx)
{
    return shaderStrings[idx];
}

size_t ShadertoyAudioProcessor::getNumShaderFiles()
{
    return shaderFiles.size();
}

bool ShadertoyAudioProcessor::hasShaderFiles()
{
    return !shaderFiles.empty();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShadertoyAudioProcessor();
}
