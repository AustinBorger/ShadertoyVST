/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
ShadertoyAudioProcessorEditor::ShadertoyAudioProcessorEditor(ShadertoyAudioProcessor &p)
 : AudioProcessorEditor(&p), 
   audioProcessor(p),
   glRenderer(p, glContext),
   patchEditor(),
   tabs(juce::TabbedButtonBar::TabsAtTop)
{
    setResizeLimits(VISU_WIDTH, VISU_HEIGHT, VISU_WIDTH, VISU_HEIGHT);
    setSize(VISU_WIDTH, VISU_HEIGHT + TAB_HEIGHT);

    addAndMakeVisible(tabs);
    tabs.setBounds(getBounds());
    tabs.addTab("Browser", juce::Colours::lightblue, &patchEditor, false);
    tabs.addTab("Visualizer", juce::Colours::lightblue, &glRenderer, false);

    patchEditor.setSize(VISU_WIDTH, VISU_HEIGHT);
    glRenderer.setSize(VISU_WIDTH, VISU_HEIGHT);
}

ShadertoyAudioProcessorEditor::~ShadertoyAudioProcessorEditor()
{
}

//==============================================================================
void ShadertoyAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill(
        juce::ColourGradient(juce::Colours::white, 0, 0,
                             juce::Colours::lightblue, 0, (float) getHeight(), false));
    g.fillAll();
}

void ShadertoyAudioProcessorEditor::resized()
{
    tabs.setBounds(getBounds());
    glRenderer.setSize(VISU_WIDTH, VISU_HEIGHT);
    patchEditor.setSize(VISU_WIDTH, VISU_HEIGHT);
}
