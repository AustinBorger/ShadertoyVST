/*
  ==============================================================================

    PluginEditor.h
    Created: 15 Aug 2020 11:22:17pm
    Author:  Austin Borger, aaborger@gmail.com

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GLRenderer.h"
#include "PatchEditor.h"
#include "Console.h"

#define ENABLE_DEBUG_CONSOLE 1

/*
 * ShadertoyAudioProcessorEditor
 *    Top-level component of the GUI. Contains a tabbed component
 *    which switches between the patch editor GUI and the OpenGL-
 *    rendered visualzation.
 */
class ShadertoyAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ShadertoyAudioProcessorEditor(ShadertoyAudioProcessor&);
    ~ShadertoyAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    inline void logDebugMessage(const juce::String &message)
    {
#if ENABLE_DEBUG_CONSOLE == 1
      debugConsole.logMessage(message);
#endif
    }

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

#if ENABLE_DEBUG_CONSOLE == 1
    Console debugConsole;
#endif

    static constexpr uint16_t UI_WIDTH = 1280;
    static constexpr uint16_t UI_HEIGHT = 720;
    static constexpr uint16_t TAB_HEIGHT = 30;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadertoyAudioProcessorEditor)
};
