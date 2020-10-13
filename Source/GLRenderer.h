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
    
    static constexpr uint16_t VISU_WIDTH = 640;
    static constexpr uint16_t VISU_HEIGHT = 360;

private:
    bool loadExtensions();
    bool buildShaderProgram(int idx);
    bool buildCopyProgram();
    bool createFramebuffer();
    bool checkIntrinsicUniform(const juce::String &name, GLenum type,
                               GLint size, bool &isIntrinsic, int programIdx);
    void alertError(const juce::String &title, const juce::String &message);

    ShadertoyAudioProcessor& processor;
    juce::OpenGLContext &glContext;
    std::vector<std::unique_ptr<juce::OpenGLShaderProgram>> programs;
    juce::OpenGLShaderProgram copyProgram;
    bool validState;
    GLuint mFramebuffer;
    GLuint mRenderTexture;
    int mFramebufferWidth;
    int mFramebufferHeight;
    std::vector<std::vector<std::unique_ptr<juce::OpenGLShaderProgram::Uniform>>> uniformFloats;
    std::vector<std::vector<std::unique_ptr<juce::OpenGLShaderProgram::Uniform>>> uniformInts;
    std::vector<std::unique_ptr<juce::OpenGLShaderProgram::Uniform>> resolutionIntrinsics;
    std::vector<int> refreshList;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> widthRatio;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> heightRatio;
    
    PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
    PFNGLDRAWBUFFERSPROC glDrawBuffers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GLRenderer)
};
