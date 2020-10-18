/*
  ==============================================================================

    GLRenderer.h
    Created: 15 Aug 2020 11:22:17pm
    Author:  Austin

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "glext.h"

//==============================================================================
/*
*/
class GLRenderer  : public juce::Component,
                    public juce::OpenGLRenderer,
                    public ShadertoyAudioProcessor::MidiListener
{
public:
    GLRenderer(ShadertoyAudioProcessor& audioProcessor,
               juce::OpenGLContext &glContext);
    ~GLRenderer() override;

    void paint (juce::Graphics&) override { }
    void resized() override;
    
    void newOpenGLContextCreated() override;
    void openGLContextClosing() override;
    void renderOpenGL() override;
  
    void handleMidiMessage(const juce::MidiMessage &message) override;

private:
    struct ProgramData {
        std::unique_ptr<juce::OpenGLShaderProgram> program;
        std::vector<std::unique_ptr<juce::OpenGLShaderProgram::Uniform>> uniformFloats;
        std::vector<std::unique_ptr<juce::OpenGLShaderProgram::Uniform>> uniformInts;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> resolutionIntrinsic;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> keyDownIntrinsic;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> keyUpIntrinsic;
    };

    bool loadExtensions();
    bool buildShaderProgram(int idx);
    bool buildCopyProgram();
    bool createFramebuffer();
    bool checkIntrinsicUniform(const juce::String &name, GLenum type,
                               GLint size, bool &isIntrinsic, int programIdx);
    void alertError(const juce::String &title, const juce::String &message);

    static constexpr int MIDI_NUM_KEYS = 128;

    ShadertoyAudioProcessor& processor;
    juce::OpenGLContext &glContext;
    juce::OpenGLShaderProgram copyProgram;
    juce::MidiMessageCollector midiCollector;
  
    std::vector<ProgramData> programData;

    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> widthRatio;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> heightRatio;

    bool validState = true;
    GLuint mFramebuffer = 0;
    GLuint mRenderTexture = 0;
    int mFramebufferWidth = 640;
    int mFramebufferHeight = 360;
    double firstRender = 0.0f;
    int samplePos = 0;
    double keyDownLast[MIDI_NUM_KEYS] = { };
    double keyUpLast[MIDI_NUM_KEYS] = { };
    
    PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
    PFNGLDRAWBUFFERSPROC glDrawBuffers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GLRenderer)
};
