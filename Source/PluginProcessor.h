/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class ShadertoyAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ShadertoyAudioProcessor();
    ~ShadertoyAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    float getUniformFloat(int i);
    int getUniformInt(int i);
    
    void addShaderFileEntry();
    void removeShaderFileEntry(int idx);
    void setShaderFile(int idx, juce::String shaderFile);
    const juce::String &getShaderFile(int idx);
    const juce::String &getShaderString(int idx);
    size_t getNumShaderFiles();
    bool hasShaderFiles();

private:
    void addUniformFloat(const juce::String &name);
    void addUniformInt(const juce::String &name);

    std::vector<std::unique_ptr<juce::AudioParameterFloat>> floatParams;
    std::vector<std::unique_ptr<juce::AudioParameterInt>> intParams;
    std::unique_ptr<juce::AudioParameterInt> programParam;
    std::vector<juce::String> shaderFiles;
    std::vector<juce::String> shaderStrings;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadertoyAudioProcessor)
};
