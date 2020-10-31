/*
  ==============================================================================

    PluginProcessor.cpp
    Created: 15 Aug 2020 11:22:17pm
    Author:  Austin Borger, aaborger@gmail.com

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ShadertoyAudioProcessor::ShadertoyAudioProcessor()
 :
#ifndef JucePlugin_PreferredChannelConfigurations
    AudioProcessor (BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                    .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                    .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                    ),
#endif
    editor(nullptr)
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

void ShadertoyAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String ShadertoyAudioProcessor::getProgramName(int index)
{
    return {};
}

void ShadertoyAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void ShadertoyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    mSampleRate = sampleRate;
    if (editor) {
        editor->logDebugMessage("prepareToPlay: sample rate " + std::to_string(mSampleRate));
    }
}

void ShadertoyAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ShadertoyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void ShadertoyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto listener : audioListeners) {
        listener->handleAudioFrame(mTimestamp, mSampleRate, buffer, midiMessages);
    }

    mTimestamp += (double)(buffer.getNumSamples()) / mSampleRate;
}

//==============================================================================
bool ShadertoyAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ShadertoyAudioProcessor::createEditor()
{
    if (this->editor != nullptr) {
        // Unlikely to impossible
        this->editor->logDebugMessage("Warning: Editor not a singleton!");
    }

    this->editor = new ShadertoyAudioProcessorEditor(*this);
    return this->editor;
}

//==============================================================================
void ShadertoyAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::XmlElement xml("ShadertoyState");

    for (int i = 0; i < shaderData.size(); i++) {
        juce::XmlElement *shaderFileElement = new juce::XmlElement("ShaderFile");
        shaderFileElement->setAttribute("Path", shaderData[i].path);
        shaderFileElement->setAttribute("Destination", shaderData[i].destination);
        
        if (shaderData[i].fixedSizeBuffer) {
            juce::XmlElement *fixedSizeData = new juce::XmlElement("FixedSizeBuffer");
            fixedSizeData->setAttribute("Width", shaderData[i].fixedSizeWidth);
            fixedSizeData->setAttribute("Height", shaderData[i].fixedSizeHeight);
            shaderFileElement->addChildElement(fixedSizeData);
        }
        
        xml.addChildElement(shaderFileElement);
    }

    juce::XmlElement *globalProperties = new juce::XmlElement("GlobalProperties");
    globalProperties->setAttribute("Width", visualizationWidth);
    globalProperties->setAttribute("Height", visualizationHeight);
    xml.addChildElement(globalProperties);
    copyXmlToBinary(xml, destData);
}

void ShadertoyAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    shaderData.clear();
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName("ShadertoyState")) {
        juce::XmlElement* child = xmlState->getFirstChildElement();
        while (child != nullptr) {
            if (child->hasTagName("ShaderFile")) {
                const juce::String &shaderFile = child->getStringAttribute("Path");
                addShaderFileEntry();
                setShaderFile(getNumShaderFiles() - 1, shaderFile);
                shaderData.back().destination =
                    child->getIntAttribute("Destination", shaderData.back().destination);

                juce::XmlElement *shaderChild = child->getFirstChildElement();
                while (shaderChild != nullptr) {
                    if (shaderChild->hasTagName("FixedSizeBuffer")) {
                        shaderData.back().fixedSizeBuffer = true;
                        shaderData.back().fixedSizeWidth =
                            shaderChild->getIntAttribute("Width", shaderData.back().fixedSizeWidth);
                        shaderData.back().fixedSizeHeight =
                            shaderChild->getIntAttribute("Height", shaderData.back().fixedSizeHeight);
                    }
                    shaderChild = shaderChild->getNextElement();
                }
            } else if (child->hasTagName("GlobalProperties")) {
                visualizationWidth = child->getIntAttribute("Width", visualizationWidth);
                visualizationHeight = child->getIntAttribute("Height", visualizationHeight);
            }
            child = child->getNextElement();
        }
    }

    for (auto *listener : stateListeners) {
        listener->processorStateChanged();
    }
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
    shaderData.emplace_back();
}

void ShadertoyAudioProcessor::removeShaderFileEntry(int idx)
{
    shaderData.erase(shaderData.begin() + idx);
}

void ShadertoyAudioProcessor::setShaderFile(int idx, juce::String shaderFile)
{
    shaderData[idx].path = std::move(shaderFile);
    reloadShaderFile(idx);
}

void ShadertoyAudioProcessor::setShaderFixedSizeBuffer(int idx, bool fixedSizeBuffer)
{
    shaderData[idx].fixedSizeBuffer = fixedSizeBuffer;
}

void ShadertoyAudioProcessor::setShaderFixedSizeWidth(int idx, int width)
{
    shaderData[idx].fixedSizeWidth = width;
}

void ShadertoyAudioProcessor::setShaderFixedSizeHeight(int idx, int height)
{
    shaderData[idx].fixedSizeHeight = height;
}

void ShadertoyAudioProcessor::setShaderDestination(int idx, int destination)
{
    shaderData[idx].destination = destination;
}

void ShadertoyAudioProcessor::reloadShaderFile(int idx)
{
    juce::File file(shaderData[idx].path);
    shaderData[idx].source = file.loadFileAsString();
}

const juce::String &ShadertoyAudioProcessor::getShaderFile(int idx)
{
    return shaderData[idx].path;
}

const juce::String &ShadertoyAudioProcessor::getShaderString(int idx)
{
    return shaderData[idx].source;
}

bool ShadertoyAudioProcessor::getShaderFixedSizeBuffer(int idx)
{
    return shaderData[idx].fixedSizeBuffer;
}

int ShadertoyAudioProcessor::getShaderFixedSizeWidth(int idx)
{
    return shaderData[idx].fixedSizeWidth;
}

int ShadertoyAudioProcessor::getShaderFixedSizeHeight(int idx)
{
    return shaderData[idx].fixedSizeHeight;
}

int ShadertoyAudioProcessor::getShaderDestination(int idx)
{
    return shaderData[idx].destination;
}

size_t ShadertoyAudioProcessor::getNumShaderFiles()
{
    return shaderData.size();
}

bool ShadertoyAudioProcessor::hasShaderFiles()
{
    return !shaderData.empty();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShadertoyAudioProcessor();
}
