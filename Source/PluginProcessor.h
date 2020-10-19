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
    class StateListener
    {
    public:
        virtual void processorStateChanged() = 0;
    };

    class MidiListener
    {
    public:
        virtual void handleMidiMessages(double timestamp, juce::MidiBuffer &midiBuffer) = 0;
    };

    //==============================================================================
    ShadertoyAudioProcessor();
    ~ShadertoyAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
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
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    float getUniformFloat(int i);
    int getUniformInt(int i);
    int getProgramIdx();

    int getVisualizationWidth()
      { return visualizationWidth; }
    void setVisualizationWidth(int width)
      { visualizationWidth = width; }
    int getVisualizationHeight()
      { return visualizationHeight; }
    void setVisualizationHeight(int height)
      { visualizationHeight = height; }
    
    void addShaderFileEntry();
    void removeShaderFileEntry(int idx);
    void setShaderFile(int idx, juce::String shaderFile);
    void setShaderFixedSizeBuffer(int idx, bool fixedSizeBuffer);
    void setShaderFixedSizeWidth(int idx, int width);
    void setShaderFixedSizeHeight(int idx, int height);
    void reloadShaderFile(int idx);
    const juce::String &getShaderFile(int idx);
    const juce::String &getShaderString(int idx);
    bool getShaderFixedSizeBuffer(int idx);
    int getShaderFixedSizeWidth(int idx);
    int getShaderFixedSizeHeight(int idX);
    size_t getNumShaderFiles();
    bool hasShaderFiles();

    void addStateListener(StateListener *listener)
    {
      stateListeners.emplace_back(listener);
    }

    void removeStateListener(StateListener *listener)
    {
      auto it = std::find(stateListeners.begin(), stateListeners.end(), listener);
      if (it != stateListeners.end()) {
        stateListeners.erase(it);
      }
    }

    void addMidiListener(MidiListener *listener)
    {
      midiListeners.emplace_back(listener);
    }

    void removeMidiListener(MidiListener *listener)
    {
      auto it = std::find(midiListeners.begin(), midiListeners.end(), listener);
      if (it != midiListeners.end()) {
        midiListeners.erase(it);
      }
    }

private:
    struct ShaderData
    {
        juce::String path;
        juce::String source;
        bool fixedSizeBuffer = false;
        int fixedSizeWidth = 640;
        int fixedSizeHeight = 360;
    };

    void addUniformFloat(const juce::String &name);
    void addUniformInt(const juce::String &name);

    std::vector<StateListener *> stateListeners;
    std::vector<MidiListener *> midiListeners;

    std::vector<std::unique_ptr<juce::AudioParameterFloat>> floatParams;
    std::vector<std::unique_ptr<juce::AudioParameterInt>> intParams;
    std::unique_ptr<juce::AudioParameterInt> programParam;
    std::vector<ShaderData> shaderData;
    int visualizationWidth = 1280;
    int visualizationHeight = 720;
    double mTimestamp = 0.0;
    double mSampleRate = 44100.0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadertoyAudioProcessor)
};
