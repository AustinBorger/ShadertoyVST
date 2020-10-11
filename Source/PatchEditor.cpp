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
               &filter, nullptr),
   shaderListBox(),
   newShaderButton()
{
    addAndMakeVisible(shaderListBox);
    shaderListBox.getHeader().addColumn("ID", 0, 30, 30, -1,
                                        juce::TableHeaderComponent::ColumnPropertyFlags::notSortable);
    shaderListBox.getHeader().addColumn("File Location", 1, 30, 30, -1,
                                        juce::TableHeaderComponent::ColumnPropertyFlags::notSortable);
                                        
    addAndMakeVisible(newShaderButton);
    newShaderButton.setButtonText("New");
    
    addAndMakeVisible(deleteButton);
    deleteButton.setButtonText("Delete");
}

PatchEditor::~PatchEditor()
{
}

void PatchEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    juce::Rectangle<int> behindShaderList = shaderListBox.getBounds();
    behindShaderList.setHeight(getHeight());
    g.setColour(shaderListBox.findColour(juce::ListBox::backgroundColourId));
    g.fillRect(behindShaderList);
}

void PatchEditor::resized()
{
    juce::Rectangle<int> shaderListBounds;
    shaderListBounds.setX(0);
    shaderListBounds.setY(0);
    shaderListBounds.setHeight(getBounds().getHeight() - 40);
    shaderListBounds.setWidth(getBounds().getWidth() / 3);

    shaderListBox.setBounds(shaderListBounds);
    shaderListBox.getHeader().setColumnWidth(0, 40);
    shaderListBox.getHeader().setColumnWidth(1, shaderListBounds.getWidth() - 40);
    
    int newButtonWidth = 75;
    int deleteButtonWidth = 75;
    int space = 10;
    int totalWidth = newButtonWidth + deleteButtonWidth + space;
    newShaderButton.setBounds((shaderListBounds.getWidth() - totalWidth) / 2, getHeight() - 30, newButtonWidth, 20);
    deleteButton.setBounds((shaderListBounds.getWidth() - totalWidth) / 2 + newButtonWidth + space, getHeight() - 30, deleteButtonWidth, 20);
}

void PatchEditor::selectionChanged()
{
    // Empty
}

void PatchEditor::fileClicked(const juce::File &file, const juce::MouseEvent &e)
{
    editor->setShader(file.getFullPathName());
}

void PatchEditor::fileDoubleClicked(const juce::File &file)
{
    // Empty
}

void PatchEditor::browserRootChanged(const juce::File &newRoot)
{
    // Empty
}
