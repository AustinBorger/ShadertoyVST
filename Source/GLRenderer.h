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
class GLRenderer  : public juce::Component, public juce::OpenGLRenderer
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
    
    void setShader(const juce::String &shaderString);
    
    static constexpr uint16_t VISU_WIDTH = 640;
    static constexpr uint16_t VISU_HEIGHT = 360;

private:
    bool loadExtensions();
    bool buildShaderProgram();
    bool buildCopyProgram();
    bool createFramebuffer();
    bool checkIntrinsicUniform(const juce::String &name, GLenum type,
                               GLint size, bool &isIntrinsic);
    void alertError(const juce::String &title, const juce::String &message);

    ShadertoyAudioProcessor& audioProcessor;
    juce::OpenGLContext &glContext;
    juce::OpenGLShaderProgram program;
    juce::OpenGLShaderProgram copyProgram;
    bool validState;
    bool newShaderProgram;
    GLuint mFramebuffer;
    GLuint mRenderTexture;
    std::vector<std::unique_ptr<juce::OpenGLShaderProgram::Uniform>> uniforms;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> resolutionIntrinsic;
    juce::String shaderString;
    
    PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
    PFNGLDRAWBUFFERSPROC glDrawBuffers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GLRenderer)
};
