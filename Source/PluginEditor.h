/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GLRenderer.h"
#include "PatchEditor.h"

//==============================================================================
/**
*/
class ShadertoyAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ShadertoyAudioProcessorEditor(ShadertoyAudioProcessor&);
    ~ShadertoyAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class BoundsConstrainer : public juce::ComponentBoundsConstrainer
    {
    public:
        void checkBounds(juce::Rectangle<int>& bounds,
                         const juce::Rectangle<int>& previousBounds,
                         const juce::Rectangle<int>& limits,
                         bool isStretchingTop,
                         bool isStretchingLeft,
                         bool isStretchingBottom,
                         bool isStretchingRight) override;
    };

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ShadertoyAudioProcessor& audioProcessor;
    juce::OpenGLContext glContext;
    GLRenderer glRenderer;
    PatchEditor patchEditor;
    juce::TabbedComponent tabs;
    BoundsConstrainer boundsConstrainer;

    static constexpr uint16_t TAB_HEIGHT = 30;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadertoyAudioProcessorEditor)
};
