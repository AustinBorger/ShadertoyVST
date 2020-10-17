/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


ShadertoyAudioProcessorEditor::ShadertoyAudioProcessorEditor(ShadertoyAudioProcessor &p)
 : AudioProcessorEditor(&p), 
   processor(p),
   glRenderer(p, glContext),
   patchEditor(this, p),
   tabs(this, juce::TabbedButtonBar::TabsAtTop)
{
    juce::Colour bgColor = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);

    setSize(UI_WIDTH, UI_HEIGHT + TAB_HEIGHT);
    setResizable(false, false);

    addAndMakeVisible(tabs);
    tabs.setBounds(getBounds());
    tabs.addTab("Patch", bgColor, &patchEditor, false);
    tabs.addTab("Visualizer", bgColor, &glRenderer, false);

    patchEditor.setBounds(0, TAB_HEIGHT, getWidth(), getHeight() - TAB_HEIGHT);
    glRenderer.setBounds(0, TAB_HEIGHT, getWidth(), getHeight() - TAB_HEIGHT);
}

ShadertoyAudioProcessorEditor::~ShadertoyAudioProcessorEditor()
{
}

void ShadertoyAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::Colour bgColor = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    g.setColour(bgColor);
    g.fillAll();
}

void ShadertoyAudioProcessorEditor::resized()
{
    tabs.setBounds(getBounds());
    glRenderer.setBounds(0, TAB_HEIGHT, getWidth(), getHeight() - TAB_HEIGHT);
    patchEditor.setBounds(0, TAB_HEIGHT, getWidth(), getHeight() - TAB_HEIGHT);
}

void ShadertoyAudioProcessorEditor::currentTabChanged(int newCurrentTabIndex)
{
  if (newCurrentTabIndex == 0) {
    setSize(UI_WIDTH, UI_HEIGHT + TAB_HEIGHT);
  } else if (newCurrentTabIndex == 1) {
    setSize(processor.getVisualizationWidth(),
            processor.getVisualizationHeight() + TAB_HEIGHT);
  }
}

ShadertoyAudioProcessorEditor::Tabs::Tabs(ShadertoyAudioProcessorEditor *editor,
                                          juce::TabbedButtonBar::Orientation orientation)
 : editor(editor),
   juce::TabbedComponent(orientation)
{ }

void ShadertoyAudioProcessorEditor::Tabs::currentTabChanged(int newCurrentTabIndex, const juce::String &newCurrentTabName)
{
  editor->currentTabChanged(newCurrentTabIndex);
}