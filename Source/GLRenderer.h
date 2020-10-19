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

#define GLRENDER_LOG_FPS 0

class ShadertoyAudioProcessorEditor;

//==============================================================================
/*
*/
class GLRenderer  : public juce::Component,
                    public juce::OpenGLRenderer,
                    public ShadertoyAudioProcessor::MidiListener
{
public:
    GLRenderer(ShadertoyAudioProcessor& processor,
               ShadertoyAudioProcessorEditor &editor,
               juce::OpenGLContext &glContext);
    ~GLRenderer() override;

    void paint (juce::Graphics&) override { }
    void resized() override;
    
    void newOpenGLContextCreated() override;
    void openGLContextClosing() override;
    void renderOpenGL() override;
  
    void handleMidiMessages(double timestamp, juce::MidiBuffer &midiBuffer) override;

private:
    struct ProgramData {
        std::unique_ptr<juce::OpenGLShaderProgram> program;
        std::vector<std::unique_ptr<juce::OpenGLShaderProgram::Uniform>> uniformFloats;
        std::vector<std::unique_ptr<juce::OpenGLShaderProgram::Uniform>> uniformInts;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> resolutionIntrinsic;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> keyDownIntrinsic;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> keyUpIntrinsic;
    };

    struct MidiFrame {
        juce::MidiBuffer buffer;
        double timestamp;
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
    ShadertoyAudioProcessorEditor &editor;
    juce::OpenGLContext &glContext;
    juce::OpenGLShaderProgram copyProgram;
    juce::CriticalSection mutex;
  
    std::vector<ProgramData> programData;

    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> widthRatio;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> heightRatio;

    bool validState = true;
    GLuint mFramebuffer = 0;
    GLuint mRenderTexture = 0;
    int mFramebufferWidth = 640;
    int mFramebufferHeight = 360;
    double keyDownLast[MIDI_NUM_KEYS] = { };
    double keyUpLast[MIDI_NUM_KEYS] = { };
    double firstRender = 0.0;
    double prevRender = 0.0;
    double firstMidiTimestamp = -1.0;

    std::queue<MidiFrame> midiFrames;

#if GLRENDER_LOG_FPS == 1
    double avgFPS = 0.0;
    double lastFPSLog = 0.0;
#endif
    
    PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
    PFNGLDRAWBUFFERSPROC glDrawBuffers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GLRenderer)
};
