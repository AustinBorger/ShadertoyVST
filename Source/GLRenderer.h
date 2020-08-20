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

private:
    bool loadExtensions();
    bool buildShaderProgram();
    bool checkIntrinsicUniform(const juce::String &name, GLenum type,
                               GLint size, bool &isIntrinsic);
    void alertError(const juce::String &title, const juce::String &message);

    ShadertoyAudioProcessor& audioProcessor;
    juce::OpenGLContext &glContext;
    juce::OpenGLShaderProgram program;
    bool validState;
    std::vector<std::unique_ptr<juce::OpenGLShaderProgram::Uniform>> uniforms;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> resolutionIntrinsic;
    
    PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GLRenderer)
};
