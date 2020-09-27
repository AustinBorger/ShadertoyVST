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

    setSize(MIN_WIDTH, MIN_HEIGHT);
    setResizable(true, true);

    addAndMakeVisible(tabs);
    tabs.setBounds(getBounds());
    tabs.addTab("Browser", juce::Colours::lightblue, &patchEditor, false);
    tabs.addTab("Visualizer", juce::Colours::lightblue, &glRenderer, false);

    patchEditor.setBounds(0, TAB_HEIGHT, getWidth(), getHeight() - TAB_HEIGHT);
    glRenderer.setBounds(0, TAB_HEIGHT, getWidth(), getHeight() - TAB_HEIGHT);
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

    if (bounds.getWidth() > MAX_WIDTH) {
        bounds.setWidth(MAX_WIDTH);
    } else if (bounds.getWidth() < MIN_WIDTH) {
        bounds.setWidth(MIN_WIDTH);
    }

    bounds.setHeight((double)bounds.getWidth() / VISU_ASPECT_RATIO + TAB_HEIGHT);
}