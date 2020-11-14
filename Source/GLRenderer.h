/*
  ==============================================================================

    GLRenderer.h
    Created: 15 Aug 2020 11:22:17pm
    Author:  Austin Borger, aaborger@gmail.com

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "glext.h"

#define GLRENDER_LOG_FPS 0

class ShadertoyAudioProcessorEditor;

/*
 * GLRenderer
 *    Renders the OpenGL-powered visualization.
 */
class GLRenderer  : public juce::Component,
                    public juce::OpenGLRenderer,
                    public ShadertoyAudioProcessor::AudioListener
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
  
    void handleAudioFrame(double timestamp, double sampleRate,
                          juce::AudioBuffer<float>& buffer,
                          juce::MidiBuffer &midiBuffer) override;

private:
    struct ProgramData {
        std::unique_ptr<juce::OpenGLShaderProgram> program;
        std::vector<std::unique_ptr<juce::OpenGLShaderProgram::Uniform>> uniformFloats;
        std::vector<std::unique_ptr<juce::OpenGLShaderProgram::Uniform>> uniformInts;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> outputResolutionIntrinsic;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> auxResolutionIntrinsic[4];
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> auxBufferIntrinsic[4];
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> keyDownIntrinsic;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> keyUpIntrinsic;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> timeIntrinsic;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> sampleRateIntrinsic;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> audioChannel0;
        GLint sizeAudioChannel0;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> audioChannel1;
        GLint sizeAudioChannel1;
    };

    struct MidiFrame {
        juce::MidiBuffer buffer;
        double timestamp;
    };

    struct Framebuffer {
        GLuint framebufferObj = 0;
        GLuint textureObj = 0;
        int width = 640;
        int height = 360;
    };

    bool loadExtensions();
    bool buildShaderProgram(int idx);
    bool buildCopyProgram();
    bool createFramebuffer(Framebuffer &fbOut,
                           int destinationId);
    bool checkIntrinsicUniform(const juce::String &name, GLenum type,
                               GLint size, bool &isIntrinsic, int programIdx);
    void setProgramIntrinsics(int programIdx,
                              double currentAudioTimestamp,
                              int backBufferWidth,
                              int backBufferHeight);
    void renderAuxBuffer(int bufferIdx,
                         double currentAudioTimestamp,
                         int backBufferWidth,
                         int backBufferHeight);
    void renderOutputBuffer(double currentAudioTimestamp,
                            int backBufferWidth,
                            int backBufferHeight);
    Framebuffer &destinationToFramebuffer(int destinationId);
    void alertError(const juce::String &title, const juce::String &message);

    /*
     * Number of keys on a midi keyboard
     */
    static constexpr int MIDI_NUM_KEYS = 128;

    /*
     * processBlock, and by extension handleAudioFrame, is called at irregular
     * intervals. To smooth out input audio and midi, we introduce an artificial
     * delay. As a result the sample position and midi timestamps provided to
     * the shaders advance at a (roughly) constant pace between frames.
     */
    static constexpr double DELAY_LATENCY = 0.016;

    ShadertoyAudioProcessor& processor;
    ShadertoyAudioProcessorEditor &editor;
    juce::OpenGLContext &glContext;
    juce::OpenGLShaderProgram copyProgram;
    juce::CriticalSection mutex;
  
    std::vector<ProgramData> programData;

    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> widthRatio;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> heightRatio;

    bool validState = true;
    double mSampleRate = 44100.0;

    Framebuffer mOutputFramebuffer; // Used if the output shader wants a fixed size
    Framebuffer mAuxFramebuffers[4]; // Buffer A, B, C, D

    // Time stuff
    double firstRender = -1.0;
    double prevRender = -1.0;
    double firstAudioTimestamp = -1.0;
    double lastAudioTimestamp = -1.0;
    double keyDownLast[MIDI_NUM_KEYS] = { };
    double keyUpLast[MIDI_NUM_KEYS] = { };
  
    std::unique_ptr<float[]> audioChannel0;
    GLint maxSizeAudioChannel0 = 0;
    GLint sizeAudioChannel0 = 0;
    std::unique_ptr<float[]> audioChannel1;
    GLint maxSizeAudioChannel1 = 0;
    GLint sizeAudioChannel1 = 0;

    std::queue<MidiFrame> midiFrames;

    /*
     * Cached audio data/metadata for the render thread
     */
    std::unique_ptr<float[]> cacheAudioChannel0;
    GLint cacheSizeAudioChannel0 = 0;
    std::unique_ptr<float[]> cacheAudioChannel1;
    GLint cacheSizeAudioChannel1 = 0;
    double cacheLastAudioTimestamp = -1.0;

#if GLRENDER_LOG_FPS == 1
    double avgFPS = 0.0;
    double lastFPSLog = 0.0;
#endif
    
    PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
    PFNGLDRAWBUFFERSPROC glDrawBuffers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GLRenderer)
};
