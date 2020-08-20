/*
  ==============================================================================

    PatchEditor.cpp
    Created: 17 Aug 2020 7:16:02pm
    Author:  Austin

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PatchEditor.h"

//==============================================================================
PatchEditor::PatchEditor()
 : filter("*.glsl", "*", "GLSL files"),
   fileBrowser(juce::FileBrowserComponent::openMode |
               juce::FileBrowserComponent::canSelectFiles,
               juce::File::getSpecialLocation(juce::File::userHomeDirectory),
               &filter, nullptr)
{
    addAndMakeVisible(fileBrowser);
    fileBrowser.setBounds(getBounds());
}

PatchEditor::~PatchEditor()
{
}

void PatchEditor::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 1);
}

void PatchEditor::resized()
{
    fileBrowser.setBounds(getBounds());
}
