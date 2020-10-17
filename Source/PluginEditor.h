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
    class Tabs : public juce::TabbedComponent
    {
    public:
      Tabs(ShadertoyAudioProcessorEditor &editor,
           juce::TabbedButtonBar::Orientation orientation);
      virtual ~Tabs() { }

      void currentTabChanged(int newCurrentTabIndex, const juce::String &newCurrentTabName) override;

    private:
      ShadertoyAudioProcessorEditor &editor;
    };

    void currentTabChanged(int newCurrentTabeIndex);

    ShadertoyAudioProcessor& processor;
    juce::OpenGLContext glContext;
    GLRenderer glRenderer;
    PatchEditor patchEditor;
    Tabs tabs;

    static constexpr uint16_t UI_WIDTH = 1280;
    static constexpr uint16_t UI_HEIGHT = 720;
    static constexpr uint16_t TAB_HEIGHT = 30;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadertoyAudioProcessorEditor)
};
