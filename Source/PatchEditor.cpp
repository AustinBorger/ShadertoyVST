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
PatchEditor::PatchEditor(ShadertoyAudioProcessorEditor *editor,
                         ShadertoyAudioProcessor& processor)
 : editor(editor),
   processor(processor),
   shaderListBox(),
   newShaderButton(),
   deleteButton(),
   shaderListBoxModel(&shaderListBox, this, processor)
{
    addAndMakeVisible(shaderListBox);
    shaderListBox.setModel(&shaderListBoxModel);
    shaderListBox.getHeader().addColumn("ID", 0, 30, 30, -1,
                                        juce::TableHeaderComponent::ColumnPropertyFlags::notSortable);
    shaderListBox.getHeader().addColumn("File Location", 1, 30, 30, -1,
                                        juce::TableHeaderComponent::ColumnPropertyFlags::notSortable);
                                        
    addAndMakeVisible(newShaderButton);
    newShaderButton.setButtonText("New");
    newShaderButton.addListener(this);
    
    addAndMakeVisible(deleteButton);
    deleteButton.setButtonText("Delete");
    deleteButton.addListener(this);
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

void PatchEditor::buttonClicked(juce::Button *button)
{
    if (button == &newShaderButton) {
        shaderListBoxModel.newRow();
    } else if (button == &deleteButton) {
        shaderListBoxModel.deleteSelectedRow();
    }
}

PatchEditor::ShaderListBoxModel::ShaderListBoxModel(juce::TableListBox *box,
                                                    PatchEditor *parent,
                                                    ShadertoyAudioProcessor &processor)
 : box(box),
   parent(parent),
   processor(processor),
   selectedRow(-1)
{ }

int PatchEditor::ShaderListBoxModel::getNumRows()
{
    return processor.getNumShaderFiles();
}

void PatchEditor::ShaderListBoxModel::paintRowBackground(
    juce::Graphics &g,
    int rowNumber,
    int width,
    int height,
    bool rowIsSelected)
{
    auto alternateColour = box->findColour(juce::ListBox::backgroundColourId)
                           .interpolatedWith(box->findColour(juce::ListBox::textColourId), 0.03f);
    if (rowIsSelected) {
        g.fillAll(juce::Colours::lightblue);
    } else if (rowNumber % 2) {
        g.fillAll(alternateColour);
    }
}

void PatchEditor::ShaderListBoxModel::paintCell(
    juce::Graphics &g,
    int rowNumber,
    int columnId,
    int width,
    int height,
    bool rowIsSelected)
{
    g.setColour(rowIsSelected ? juce::Colours::darkblue : box->findColour(juce::ListBox::textColourId));
    g.setFont(font);
    
    if (columnId == 0) {
        g.drawText(std::to_string(rowNumber), 2, 0, width - 4, height, juce::Justification::centredRight, true);
    } else if (columnId == 1) {
        juce::String text = "<none>";
        if (!processor.getShaderFile(rowNumber).isEmpty()) {
            text = processor.getShaderFile(rowNumber);
        }

        g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
 
    g.setColour(box->findColour(juce::ListBox::backgroundColourId));
    g.fillRect(width - 1, 0, 1, height);
}

void PatchEditor::ShaderListBoxModel::selectedRowsChanged(int lastRowSelected)
{
    selectedRow = lastRowSelected;
}

void PatchEditor::ShaderListBoxModel::cellDoubleClicked(
    int rowNumber,
    int columnId,
    const juce::MouseEvent &)
{
    juce::FileChooser fileChooser("Choose Shader File", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.glsl");
    if (fileChooser.browseForFileToOpen()) {
        processor.setShaderFile(rowNumber, fileChooser.getResult().getFullPathName());
        box->updateContent();
    }
}

void PatchEditor::ShaderListBoxModel::newRow()
{
    processor.addShaderFileEntry();
    box->updateContent();
}

void PatchEditor::ShaderListBoxModel::deleteSelectedRow()
{
    if (selectedRow > -1) {
        processor.removeShaderFileEntry(selectedRow);
        box->updateContent();
    }
}