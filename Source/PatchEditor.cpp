/*
  ==============================================================================

    PatchEditor.cpp
    Created: 17 Aug 2020 7:16:02pm
    Author:  Austin

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PatchEditor.h"
#include "PluginEditor.h"

//==============================================================================
PatchEditor::PatchEditor(ShadertoyAudioProcessorEditor *editor)
 : editor(editor),
   filter("*.glsl", "*", "GLSL files"),
   fileBrowser(juce::FileBrowserComponent::openMode |
               juce::FileBrowserComponent::canSelectFiles,
               juce::File::getSpecialLocation(juce::File::userHomeDirectory),
               &filter, nullptr)
{
    addAndMakeVisible(fileBrowser);
    fileBrowser.setBounds(getBounds());
    fileBrowser.addListener(this);
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

void PatchEditor::selectionChanged()
{
    // Empty
}

void PatchEditor::fileClicked(const juce::File &file, const juce::MouseEvent &e)
{
    juce::String contents = file.loadFileAsString();
    editor->setShader(contents);
}

void PatchEditor::fileDoubleClicked(const juce::File &file)
{
    // Empty
}

void PatchEditor::browserRootChanged(const juce::File &newRoot)
{
    // Empty
}