/*
  ==============================================================================

    PluginProcessor.h
    Created: 15 Aug 2020 11:22:17pm
    Author:  Austin Borger, aaborger@gmail.com

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ShadertoyAudioProcessorEditor;

/*
 * ShadertoyAudioProcessor
 *    The audio processor. This is responsible for saving / loading patches
 *    and directing audio / midi / parameter input to the visualization.
 */
class ShadertoyAudioProcessor  : public juce::AudioProcessor
{
public:
    class StateListener
    {
    public:
        virtual void processorStateChanged() = 0;
    };

    class AudioListener
    {
    public:
        virtual void handleAudioFrame(double timestamp, double sampleRate,
                                      juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer &midiBuffer) = 0;
    };

    ShadertoyAudioProcessor();
    ~ShadertoyAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    float getUniformFloat(int i);
    int getUniformInt(int i);
  
    int getOutputProgramIdx();

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
    void setShaderDestination(int idx, int destination);
    void reloadShaderFile(int idx);
    const juce::String &getShaderFile(int idx);
    const juce::String &getShaderString(int idx);
    bool getShaderFixedSizeBuffer(int idx);
    int getShaderFixedSizeWidth(int idx);
    int getShaderFixedSizeHeight(int idx);
    int getShaderDestination(int idx);
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

    void addAudioListener(AudioListener *listener)
    {
      audioListeners.emplace_back(listener);
    }

    void removeAudioListener(AudioListener *listener)
    {
      auto it = std::find(audioListeners.begin(), audioListeners.end(), listener);
      if (it != audioListeners.end()) {
        audioListeners.erase(it);
      }
    }

    void editorFreed() { this->editor = nullptr; }

private:
    struct ShaderData
    {
        juce::String path;
        juce::String source;
        bool fixedSizeBuffer = false;
        int fixedSizeWidth = 640;
        int fixedSizeHeight = 360;
        int destination = 1;
    };

    void addUniformFloat(const juce::String &name);
    void addUniformInt(const juce::String &name);

    ShadertoyAudioProcessorEditor *editor;

    std::vector<StateListener *> stateListeners;
    std::vector<AudioListener *> audioListeners;

    std::vector<std::unique_ptr<juce::AudioParameterFloat>> floatParams;
    std::vector<std::unique_ptr<juce::AudioParameterInt>> intParams;
    std::unique_ptr<juce::AudioParameterInt> outputProgramParam;
    std::vector<ShaderData> shaderData;
    int visualizationWidth = 1280;
    int visualizationHeight = 720;
    double mTimestamp = 0.0;
    double mSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadertoyAudioProcessor)
};
