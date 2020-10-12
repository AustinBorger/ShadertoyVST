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
   patchEditor(this, p),
   tabs(juce::TabbedButtonBar::TabsAtTop)
{
    int width = GLRenderer::VISU_WIDTH * 2;
    int height = getAppropriateHeight(width);
    juce::Colour bgColor = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);

    setSize(width, height);
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

//==============================================================================
void ShadertoyAudioProcessorEditor::paint (juce::Graphics& g)
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

int ShadertoyAudioProcessorEditor::getAppropriateHeight(int width)
{
    static constexpr double VISU_ASPECT_RATIO =
        (double)GLRenderer::VISU_WIDTH / (double)GLRenderer::VISU_HEIGHT;
    return (int)((double)width / VISU_ASPECT_RATIO + TAB_HEIGHT);
}