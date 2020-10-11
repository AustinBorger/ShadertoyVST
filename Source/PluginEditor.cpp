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
   patchEditor(this),
   tabs(juce::TabbedButtonBar::TabsAtTop),
   boundsConstrainer()
{
    setConstrainer(&boundsConstrainer);
    
    int width = GLRenderer::VISU_WIDTH * 2;
    int height = getAppropriateHeight(width);

    setSize(width, height);
    setResizable(false, false);

    addAndMakeVisible(tabs);
    tabs.setBounds(getBounds());
    tabs.addTab("Patch", juce::Colours::grey, &patchEditor, false);
    tabs.addTab("Visualizer", juce::Colours::grey, &glRenderer, false);

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
        juce::ColourGradient(juce::Colours::grey, 0, 0,
                             juce::Colours::grey, 0, (float) getHeight(), false));
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

    bounds.setHeight(getAppropriateHeight(bounds.getWidth()));
}

void
ShadertoyAudioProcessorEditor::setShader(const juce::String &shaderPath)
{
    glRenderer.setShader(shaderPath);
}