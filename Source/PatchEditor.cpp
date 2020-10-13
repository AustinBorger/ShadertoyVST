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
   reloadButton(),
   fixedSizeButton("Fixed Size Framebuffer"),
   fixedSizeWidthEditor(),
   fixedSizeWidthLabel(),
   fixedSizeHeightEditor(),
   fixedSizeHeightLabel(),
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
    
    addAndMakeVisible(reloadButton);
    reloadButton.setButtonText("Reload");
    reloadButton.addListener(this);
    
    addAndMakeVisible(fixedSizeButton);
    fixedSizeButton.addListener(this);
    
    addAndMakeVisible(fixedSizeWidthEditor);
    fixedSizeWidthEditor.setMultiLine(false);
    fixedSizeWidthEditor.setInputRestrictions(4, "0123456789");
    fixedSizeWidthEditor.addListener(this);
    
    addAndMakeVisible(fixedSizeWidthLabel);
    fixedSizeWidthLabel.setText("Width:", juce::NotificationType::dontSendNotification);
    
    addAndMakeVisible(fixedSizeHeightEditor);
    fixedSizeHeightEditor.setMultiLine(false);
    fixedSizeHeightEditor.setInputRestrictions(4, "0123456789");
    fixedSizeHeightEditor.addListener(this);
    
    addAndMakeVisible(fixedSizeHeightLabel);
    fixedSizeHeightLabel.setText("Height:", juce::NotificationType::dontSendNotification);
    
    greyOutTopRightRegion();
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

    juce::Rectangle<int> topRightRegion;
    topRightRegion.setX(behindShaderList.getWidth());
    topRightRegion.setY(0);
    topRightRegion.setWidth(getWidth() - behindShaderList.getWidth());
    topRightRegion.setHeight(getHeight() / 2);
    g.setColour(juce::Colour::fromRGB(128, 128, 128));
    g.fillRect(topRightRegion);
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
    
    int buttonWidth = 75;
    int space = 10;
    int totalWidth = buttonWidth + space + buttonWidth + space + buttonWidth;
    int newShaderX = (shaderListBounds.getWidth() - totalWidth) / 2;
    int deleteX = newShaderX + buttonWidth + space;
    int reloadX = deleteX + buttonWidth + space;
    newShaderButton.setBounds(newShaderX, getHeight() - 30, buttonWidth, 20);
    deleteButton.setBounds(deleteX, getHeight() - 30, buttonWidth, 20);
    reloadButton.setBounds(reloadX, getHeight() - 30, buttonWidth, 20);
    
    juce::Rectangle<int> topRightRegion;
    topRightRegion.setX(shaderListBounds.getWidth());
    topRightRegion.setY(0);
    topRightRegion.setWidth(getWidth() - shaderListBounds.getWidth());
    topRightRegion.setHeight(getHeight() / 2);
    
    int padding = 10;
    int fixedSizeButtonHeight = 20;
    fixedSizeButton.setBounds(topRightRegion.getX() + padding,
                              topRightRegion.getY() + padding,
                              175, fixedSizeButtonHeight);
                              
    fixedSizeWidthLabel.setBounds(topRightRegion.getX() + padding, topRightRegion.getY() + padding + fixedSizeButtonHeight + space,
                                  60, 20);
    fixedSizeWidthEditor.setBounds(topRightRegion.getX() + padding + 60, topRightRegion.getY() + 40,
                                   75, 20);
    fixedSizeHeightLabel.setBounds(topRightRegion.getX() + padding, topRightRegion.getY() + padding + fixedSizeButtonHeight + space + 20 + space,
                                   75, 20);
    fixedSizeHeightEditor.setBounds(topRightRegion.getX() + padding + 60, topRightRegion.getY() + padding + fixedSizeButtonHeight + space + 20 + space,
                                    75, 20);
}

void PatchEditor::buttonClicked(juce::Button *button)
{
    if (button == &newShaderButton) {
        shaderListBoxModel.newRow();
    } else if (button == &deleteButton) {
        shaderListBoxModel.deleteSelectedRow();
    } else if (button == &reloadButton) {
        shaderListBoxModel.reloadSelectedRow();
    } else if (button == &fixedSizeButton) {
        if (fixedSizeButton.getToggleState()) {
            activateFixedSizeEditors();
        } else {
            greyOutFixedSizeEditors();
        }
        processor.setShaderFixedSizeBuffer(shaderListBoxModel.getSelectedRow(),
                                           fixedSizeButton.getToggleState());
    }
}

void PatchEditor::textEditorTextChanged(juce::TextEditor &textEditor)
{
    if (&textEditor == &fixedSizeWidthEditor) {
        int width = textEditor.getText().getIntValue();
        processor.setShaderFixedSizeWidth(shaderListBoxModel.getSelectedRow(), width);
    } else if (&textEditor == &fixedSizeHeightEditor) {
        int height = textEditor.getText().getIntValue();
        processor.setShaderFixedSizeHeight(shaderListBoxModel.getSelectedRow(), height);
    }
}

void PatchEditor::greyOutFixedSizeEditors()
{
    if (fixedSizeWidthEditor.isReadOnly()) {
        return;
    }

    juce::Colour fixedSizeWidthBg = fixedSizeWidthEditor.findColour(juce::TextEditor::backgroundColourId);
    juce::Colour fixedSizeHeightBg = fixedSizeHeightEditor.findColour(juce::TextEditor::backgroundColourId);
    
    fixedSizeWidthEditor.setColour(juce::TextEditor::backgroundColourId,
                                   juce::Colour::fromRGB(255 - (255 - fixedSizeWidthBg.getRed()) / 2,
                                                         255 - (255 - fixedSizeWidthBg.getGreen()) / 2,
                                                         255 - (255 - fixedSizeWidthBg.getBlue()) / 2));
    fixedSizeHeightEditor.setColour(juce::TextEditor::backgroundColourId,
                                    juce::Colour::fromRGB(255 - (255 - fixedSizeHeightBg.getRed()) / 2,
                                                          255 - (255 - fixedSizeHeightBg.getGreen()) / 2,
                                                          255 - (255 - fixedSizeHeightBg.getBlue()) / 2));
                                                          
    fixedSizeWidthEditor.setReadOnly(true);
    fixedSizeHeightEditor.setReadOnly(true);
    fixedSizeWidthEditor.setText("", false);
    fixedSizeHeightEditor.setText("", false);
}

void PatchEditor::activateFixedSizeEditors()
{
    if (!fixedSizeWidthEditor.isReadOnly()) {
        return;
    }

    juce::Colour fixedSizeWidthBg = fixedSizeWidthEditor.findColour(juce::TextEditor::backgroundColourId);
    juce::Colour fixedSizeHeightBg = fixedSizeHeightEditor.findColour(juce::TextEditor::backgroundColourId);
    
    fixedSizeWidthEditor.setColour(juce::TextEditor::backgroundColourId,
                                   juce::Colour::fromRGB(255 - (255 - fixedSizeWidthBg.getRed()) * 2,
                                                         255 - (255 - fixedSizeWidthBg.getGreen()) * 2,
                                                         255 - (255 - fixedSizeWidthBg.getBlue()) * 2));
    fixedSizeHeightEditor.setColour(juce::TextEditor::backgroundColourId,
                                    juce::Colour::fromRGB(255 - (255 - fixedSizeHeightBg.getRed()) * 2,
                                                          255 - (255 - fixedSizeHeightBg.getGreen()) * 2,
                                                          255 - (255 - fixedSizeHeightBg.getBlue()) * 2));
                                             
    if (shaderListBoxModel.getSelectedRow() > -1) {
        fixedSizeWidthEditor.setText(
            std::to_string(processor.getShaderFixedSizeWidth(shaderListBoxModel.getSelectedRow())));
        fixedSizeHeightEditor.setText(
            std::to_string(processor.getShaderFixedSizeHeight(shaderListBoxModel.getSelectedRow())));
    }

    fixedSizeWidthEditor.setReadOnly(false);
    fixedSizeHeightEditor.setReadOnly(false);
}

void PatchEditor::greyOutTopRightRegion()
{
    fixedSizeButton.setEnabled(false);
    
    if (!fixedSizeWidthEditor.isReadOnly()) {
        greyOutFixedSizeEditors();
    }
}

void PatchEditor::loadTopRightRegion(int shaderIdx)
{
    fixedSizeButton.setEnabled(true);
    fixedSizeButton.setToggleState(processor.getShaderFixedSizeBuffer(shaderIdx),
                                   juce::NotificationType::dontSendNotification);
    
    if (fixedSizeButton.getToggleState()) {
        activateFixedSizeEditors();
    } else {
        greyOutFixedSizeEditors();
    }
    
    if (processor.getShaderFixedSizeBuffer(shaderIdx)) {
        fixedSizeWidthEditor.setText(std::to_string(processor.getShaderFixedSizeWidth(shaderIdx)));
        fixedSizeHeightEditor.setText(std::to_string(processor.getShaderFixedSizeHeight(shaderIdx)));
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
    parent->loadTopRightRegion(selectedRow);
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
    juce::FileChooser fileChooser("Choose Shader File", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.glsl");
    if (fileChooser.browseForFileToOpen()) {
        processor.addShaderFileEntry();
        processor.setShaderFile(processor.getNumShaderFiles() - 1, fileChooser.getResult().getFullPathName());
        box->updateContent();
    }
}

void PatchEditor::ShaderListBoxModel::deleteSelectedRow()
{
    if (selectedRow > -1) {
        processor.removeShaderFileEntry(selectedRow);
        box->updateContent();
    }
}

void PatchEditor::ShaderListBoxModel::reloadSelectedRow()
{
    if (selectedRow > -1) {
        processor.reloadShaderFile(selectedRow);
        box->updateContent();
    }
}

int PatchEditor::ShaderListBoxModel::getSelectedRow()
{
    return selectedRow;
}
