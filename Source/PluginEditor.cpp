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
   tabs(juce::TabbedButtonBar::TabsAtTop),
   boundsConstrainer()
{
    setConstrainer(&boundsConstrainer);
    boundsConstrainer.setMinimumWidth(GLRenderer::VISU_WIDTH);
    boundsConstrainer.setMinimumHeight(GLRenderer::VISU_HEIGHT + TAB_HEIGHT);
    boundsConstrainer.setMaximumWidth(GLRenderer::VISU_WIDTH * 8);
    boundsConstrainer.setMaximumHeight(GLRenderer::VISU_HEIGHT * 8 + TAB_HEIGHT);
    setSize(GLRenderer::VISU_WIDTH, GLRenderer::VISU_HEIGHT + TAB_HEIGHT);
    setResizable(true, true);

    addAndMakeVisible(tabs);
    tabs.setBounds(getBounds());
    tabs.addTab("Browser", juce::Colours::lightblue, &patchEditor, false);
    tabs.addTab("Visualizer", juce::Colours::lightblue, &glRenderer, false);

    patchEditor.setBounds(0, TAB_HEIGHT, GLRenderer::VISU_WIDTH, GLRenderer::VISU_HEIGHT);
    glRenderer.setBounds(0, TAB_HEIGHT, GLRenderer::VISU_WIDTH, GLRenderer::VISU_HEIGHT);
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
    glRenderer.setBounds(0, TAB_HEIGHT, getWidth(), getHeight() - TAB_HEIGHT);
    patchEditor.setBounds(0, TAB_HEIGHT, getWidth(), getHeight() - TAB_HEIGHT);
}


void
ShadertoyAudioProcessorEditor::BoundsConstrainer::checkBounds(
    juce::Rectangle<int>& bounds,
    const juce::Rectangle<int>& previousBounds,
    const juce::Rectangle<int>& limits,
    bool isStretchingTop,
    bool isStretchingLeft,
    bool isStretchingBottom,
    bool isStretchingRight)
{
    static constexpr double VISU_ASPECT_RATIO =
        (double)GLRenderer::VISU_WIDTH / (double)GLRenderer::VISU_HEIGHT;
    bounds.setHeight((double)bounds.getWidth() / VISU_ASPECT_RATIO + TAB_HEIGHT);
}