/*
  ==============================================================================

    PatchEditor.cpp
    Created: 17 Aug 2020 7:16:02pm
    Author:  Austin Borger, aaborger@gmail.com

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PatchEditor.h"
#include "PluginEditor.h"

//==============================================================================
PatchEditor::PatchEditor(ShadertoyAudioProcessorEditor &editor, // IN / OUT
                         ShadertoyAudioProcessor& processor)    // IN / OUT
 : editor(editor),
   processor(processor),
   shaderListComponent(editor, processor, *this),
   shaderPropertiesComponent(editor, processor, *this, shaderListComponent),
   globalPropertiesComponent(editor, processor, *this)
{
    addAndMakeVisible(shaderListComponent);
    addAndMakeVisible(shaderPropertiesComponent);
    addAndMakeVisible(globalPropertiesComponent);

    processor.addStateListener(this);
}

PatchEditor::~PatchEditor()
{
    processor.removeStateListener(this);
}

void
PatchEditor::paint(juce::Graphics& g) // IN
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void
PatchEditor::resized()
{
    shaderListComponent.setBounds(0, 0, getWidth() / 3, getHeight());

    shaderPropertiesComponent.setBounds(shaderListComponent.getWidth(), 0,
                                        getWidth() - shaderListComponent.getWidth(),
                                        getHeight() / 2);

    globalPropertiesComponent.setBounds(shaderListComponent.getWidth(),
                                        shaderPropertiesComponent.getHeight(),
                                        shaderPropertiesComponent.getWidth(),
                                        getHeight() / 2);
}

void
PatchEditor::loadTopRightRegion(int shaderIdx) // IN
{
    shaderPropertiesComponent.load(shaderIdx);
}

void
PatchEditor::greyOutTopRightRegion()
{
    shaderPropertiesComponent.greyOut();
}

void
PatchEditor::processorStateChanged()
{
    globalPropertiesComponent.updateVisuSize();
}

PatchEditor::ShaderListBoxModel::ShaderListBoxModel(
    juce::TableListBox *box,            // IN / OUT
    PatchEditor &patchEditor,           // IN / OUT
    ShadertoyAudioProcessor &processor) // IN / OUT
 : box(box),
   patchEditor(patchEditor),
   processor(processor),
   selectedRow(-1)
{ }

int
PatchEditor::ShaderListBoxModel::getNumRows()
{
    return (int)processor.getNumShaderFiles();
}

void
PatchEditor::ShaderListBoxModel::paintRowBackground(
    juce::Graphics &g,  // IN
    int rowNumber,      // IN
    int width,          // IN
    int height,         // IN
    bool rowIsSelected) // IN
{
    auto alternateColour = box->findColour(juce::ListBox::backgroundColourId)
                           .interpolatedWith(box->findColour(juce::ListBox::textColourId), 0.03f);
    if (rowIsSelected) {
        g.fillAll(juce::Colours::lightblue);
    } else if (rowNumber % 2) {
        g.fillAll(alternateColour);
    }

    // Unreferenced parameters
    (void)(width);
    (void)(height);
}

void
PatchEditor::ShaderListBoxModel::paintCell(
    juce::Graphics &g,  // IN
    int rowNumber,      // IN
    int columnId,       // IN
    int width,          // IN
    int height,         // IN
    bool rowIsSelected) // IN
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

void
PatchEditor::ShaderListBoxModel::selectedRowsChanged(int lastRowSelected) // IN
{
    selectedRow = lastRowSelected;
    patchEditor.loadTopRightRegion(selectedRow);
}

void
PatchEditor::ShaderListBoxModel::cellDoubleClicked(
    int rowNumber,            // IN
    int columnId,             // IN
    const juce::MouseEvent &) // IN
{
    juce::FileChooser fileChooser("Choose Shader File", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.glsl");
    if (fileChooser.browseForFileToOpen()) {
        processor.setShaderFile(rowNumber, fileChooser.getResult().getFullPathName());
        box->updateContent();
    }

    // Unreferenced parameters
    (void)(columnId);
}

void
PatchEditor::ShaderListBoxModel::newRow()
{
    juce::FileChooser fileChooser("Choose Shader File", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.glsl");
    if (fileChooser.browseForFileToOpen()) {
        processor.addShaderFileEntry();
        processor.setShaderFile((int)processor.getNumShaderFiles() - 1, fileChooser.getResult().getFullPathName());
        box->updateContent();
    }
}

void
PatchEditor::ShaderListBoxModel::deleteSelectedRow()
{
    if (selectedRow > -1) {
        processor.removeShaderFileEntry(selectedRow);
        box->updateContent();
    }
}

void
PatchEditor::ShaderListBoxModel::reloadSelectedRow()
{
    if (selectedRow > -1) {
        processor.reloadShaderFile(selectedRow);
        box->updateContent();
    }
}

int
PatchEditor::ShaderListBoxModel::getSelectedRow()
{
    return selectedRow;
}

PatchEditor::ShaderListComponent::ShaderListComponent(
    ShadertoyAudioProcessorEditor &editor, // IN / OUT
    ShadertoyAudioProcessor &processor,    // IN / OUT
    PatchEditor &parent)                   // IN / OUT
 : editor(editor),
   processor(processor),
   parent(parent),
   shaderListBoxModel(&shaderListBox, parent, processor)
{
    addAndMakeVisible(shaderListLabel);
    shaderListLabel.setText("Shaders", juce::NotificationType::dontSendNotification);

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

    addAndMakeVisible(reloadAllButton);
    reloadAllButton.setButtonText("Reload All");
    reloadAllButton.addListener(this);
}

void
PatchEditor::ShaderListComponent::paint(juce::Graphics& g) // IN
{
    g.setColour(shaderListBox.findColour(juce::ListBox::backgroundColourId));
    g.fillAll();

    g.setColour(juce::Colours::black);
    g.fillRect(shaderListLabel.getBounds());
}

void
PatchEditor::ShaderListComponent::resized()
{
    shaderListLabel.setBounds(getX(), 0, getWidth(), 30);

    shaderListBox.setBounds(getX(), shaderListLabel.getHeight(), getWidth(),
                            getHeight() - shaderListLabel.getHeight() - 40);
    shaderListBox.getHeader().setColumnWidth(0, 40);
    shaderListBox.getHeader().setColumnWidth(1, getWidth() - 40);
    
    int buttonWidth = 75;
    int spacing = 10;
    int totalWidth = buttonWidth * 4 + spacing * 3;
    int newShaderX = (getWidth() - totalWidth) / 2;
    int deleteX = newShaderX + buttonWidth + spacing;
    int reloadX = deleteX + buttonWidth + spacing;
    int reloadAllX = reloadX + buttonWidth + spacing;
    newShaderButton.setBounds(newShaderX, getHeight() - 30, buttonWidth, 20);
    deleteButton.setBounds(deleteX, getHeight() - 30, buttonWidth, 20);
    reloadButton.setBounds(reloadX, getHeight() - 30, buttonWidth, 20);
    reloadAllButton.setBounds(reloadAllX, getHeight() - 30, buttonWidth, 20);
}

void
PatchEditor::ShaderListComponent::buttonClicked(juce::Button *button) // IN
{
    if (button == &newShaderButton) {
        shaderListBoxModel.newRow();
    } else if (button == &deleteButton) {
        shaderListBoxModel.deleteSelectedRow();
    } else if (button == &reloadButton) {
        shaderListBoxModel.reloadSelectedRow();
    } else if (button == &reloadAllButton) {
        for (int i = 0; i < processor.getNumShaderFiles(); i++) {
            processor.reloadShaderFile(i);
        }
    }
}

int
PatchEditor::ShaderListComponent::getSelectedRow()
{
    return shaderListBoxModel.getSelectedRow();
}

PatchEditor::ShaderPropertiesComponent::ShaderPropertiesComponent(
    ShadertoyAudioProcessorEditor &editor,    // IN / OUT
    ShadertoyAudioProcessor &processor,       // IN / OUT
    PatchEditor &parent,                      // IN / OUT
    ShaderListComponent &shaderListComponent) // IN / OUT
 : editor(editor),
   processor(processor),
   parent(parent),
   shaderListComponent(shaderListComponent),
   fixedSizeButton("Fixed Size Framebuffer")
{
    addAndMakeVisible(shaderPropertiesLabel);
    shaderPropertiesLabel.setText("Shader Properties", juce::NotificationType::dontSendNotification);
    shaderPropertiesLabel.setColour(juce::Label::textColourId, juce::Colours::black);

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

    addAndMakeVisible(destinationBox);
    destinationBox.addItem("Output", 1);
    destinationBox.addItem("Buffer A", 2);
    destinationBox.addItem("Buffer B", 3);
    destinationBox.addItem("Buffer C", 4);
    destinationBox.addItem("Buffer D", 5);
    destinationBox.setEnabled(false);
    destinationBox.addListener(this);

    addAndMakeVisible(destinationLabel);
    destinationLabel.setText("Destination:", juce::NotificationType::dontSendNotification);
    
    parent.greyOutTopRightRegion();
}

void
PatchEditor::ShaderPropertiesComponent::paint(juce::Graphics& g) // IN
{
    g.setColour(juce::Colour::fromRGB(128, 128, 128));
    g.fillAll();

    g.setColour(juce::Colours::white);
    g.fillRect(shaderPropertiesLabel.getBounds());
}

void
PatchEditor::ShaderPropertiesComponent::resized()
{
    shaderPropertiesLabel.setBounds(0, 0, getWidth(), 30);
    
    int padding = 10;
    int spacing = 10;
    fixedSizeButton.setBounds(padding,
                              shaderPropertiesLabel.getHeight() + padding,
                              175, 20);
                              
    fixedSizeWidthLabel.setBounds(padding,
                                  fixedSizeButton.getY() + fixedSizeButton.getHeight() + spacing,
                                  65, 20);
    fixedSizeWidthEditor.setBounds(fixedSizeWidthLabel.getX() + fixedSizeWidthLabel.getWidth(),
                                   fixedSizeWidthLabel.getY(), 75, 20);

    fixedSizeHeightLabel.setBounds(padding,
                                   fixedSizeWidthLabel.getY() + fixedSizeWidthLabel.getHeight() + spacing,
                                   65, 20);
    fixedSizeHeightEditor.setBounds(fixedSizeHeightLabel.getX() + fixedSizeHeightLabel.getWidth(),
                                    fixedSizeHeightLabel.getY(), 75, 20);

    destinationLabel.setBounds(padding,
                               fixedSizeHeightEditor.getY() + fixedSizeHeightEditor.getHeight() + spacing,
                               90, 20);
    destinationBox.setBounds(destinationLabel.getX() + destinationLabel.getWidth(),
                             destinationLabel.getY(), 100, 20);
}

void
PatchEditor::ShaderPropertiesComponent::buttonClicked(juce::Button *button) // IN
{
    if (button == &fixedSizeButton) {
        if (fixedSizeButton.getToggleState()) {
            activateFixedSizeEditors();
        } else {
            greyOutFixedSizeEditors();
        }
        processor.setShaderFixedSizeBuffer(shaderListComponent.getSelectedRow(),
                                           fixedSizeButton.getToggleState());
    }
}

void
PatchEditor::ShaderPropertiesComponent::greyOutFixedSizeEditors()
{
    if (fixedSizeWidthEditor.isReadOnly()) {
        return;
    }

    juce::Colour fixedSizeWidthBg =
        fixedSizeWidthEditor.findColour(juce::TextEditor::backgroundColourId);
    juce::Colour fixedSizeHeightBg =
        fixedSizeHeightEditor.findColour(juce::TextEditor::backgroundColourId);
    
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

void
PatchEditor::ShaderPropertiesComponent::activateFixedSizeEditors()
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
                                             
    if (shaderListComponent.getSelectedRow() > -1) {
        fixedSizeWidthEditor.setText(
            std::to_string(processor.getShaderFixedSizeWidth(shaderListComponent.getSelectedRow())), false);
        fixedSizeHeightEditor.setText(
            std::to_string(processor.getShaderFixedSizeHeight(shaderListComponent.getSelectedRow())), false);
    }

    fixedSizeWidthEditor.setReadOnly(false);
    fixedSizeHeightEditor.setReadOnly(false);
}

void
PatchEditor::ShaderPropertiesComponent::greyOut()
{
    fixedSizeButton.setEnabled(false);
    
    if (!fixedSizeWidthEditor.isReadOnly()) {
        greyOutFixedSizeEditors();
    }

    destinationBox.setEnabled(false);
    destinationBox.setSelectedId(0, juce::NotificationType::dontSendNotification);
}

void
PatchEditor::ShaderPropertiesComponent::load(int shaderIdx) // IN
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
        fixedSizeWidthEditor.setText(std::to_string(processor.getShaderFixedSizeWidth(shaderIdx)), false);
        fixedSizeHeightEditor.setText(std::to_string(processor.getShaderFixedSizeHeight(shaderIdx)), false);
    }

    destinationBox.setEnabled(true);
    destinationBox.setSelectedId(processor.getShaderDestination(shaderIdx),
                                 juce::NotificationType::dontSendNotification);
}

void
PatchEditor::ShaderPropertiesComponent::textEditorTextChanged(
    juce::TextEditor &textEditor) // IN
{
    if (&textEditor == &fixedSizeWidthEditor) {
        int width = textEditor.getText().getIntValue();
        processor.setShaderFixedSizeWidth(shaderListComponent.getSelectedRow(), width);
    } else if (&textEditor == &fixedSizeHeightEditor) {
        int height = textEditor.getText().getIntValue();
        processor.setShaderFixedSizeHeight(shaderListComponent.getSelectedRow(), height);
    }
}

void
PatchEditor::ShaderPropertiesComponent::comboBoxChanged(
    juce::ComboBox *comboBoxThatHasChanged) // IN
{
    if (comboBoxThatHasChanged == &destinationBox) {
        int id = destinationBox.getSelectedId();
        processor.setShaderDestination(shaderListComponent.getSelectedRow(), id);
    }
}

PatchEditor::GlobalPropertiesComponent::GlobalPropertiesComponent(
    ShadertoyAudioProcessorEditor &editor, // IN / OUT
    ShadertoyAudioProcessor &processor,    // IN / OUT
    PatchEditor &parent)                   // IN / OUT
 : editor(editor),
   processor(processor),
   parent(parent)
{
    addAndMakeVisible(globalPropertiesLabel);
    globalPropertiesLabel.setText("Global Properties", juce::NotificationType::dontSendNotification);
    globalPropertiesLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    addAndMakeVisible(visuWidthEditor);
    visuWidthEditor.setMultiLine(false);
    visuWidthEditor.setInputRestrictions(4, "0123456789");
    visuWidthEditor.addListener(this);
    visuWidthEditor.setText(std::to_string(processor.getVisualizationWidth()), false);

    addAndMakeVisible(visuWidthLabel);
    visuWidthLabel.setText("Visualization Width:", juce::NotificationType::dontSendNotification);

    addAndMakeVisible(visuHeightEditor);
    visuHeightEditor.setMultiLine(false);
    visuHeightEditor.setInputRestrictions(4, "0123456789");
    visuHeightEditor.addListener(this);
    visuHeightEditor.setText(std::to_string(processor.getVisualizationHeight()), false);

    addAndMakeVisible(visuHeightLabel);
    visuHeightLabel.setText("Visualization Height:", juce::NotificationType::dontSendNotification);
}

void
PatchEditor::GlobalPropertiesComponent::paint(juce::Graphics& g) // IN
{
    g.setColour(juce::Colours::white);
    g.fillRect(globalPropertiesLabel.getBounds());
}

void
PatchEditor::GlobalPropertiesComponent::resized()
{
    globalPropertiesLabel.setBounds(0, 0, getWidth(), 30);

    int padding = 10;
    int spacing = 10;
    visuWidthLabel.setBounds(padding, globalPropertiesLabel.getHeight() + padding,
                             150, 20);
    visuWidthEditor.setBounds(visuWidthLabel.getX() + visuWidthLabel.getWidth(),
                              visuWidthLabel.getY(), 75, 20);

    visuHeightLabel.setBounds(padding,
                              visuWidthLabel.getY() + visuWidthLabel.getHeight() + spacing,
                              150, 20);
    visuHeightEditor.setBounds(visuHeightLabel.getX() + visuHeightLabel.getWidth(),
                               visuHeightLabel.getY(), 75, 20);
}

void
PatchEditor::GlobalPropertiesComponent::textEditorTextChanged(
    juce::TextEditor &textEditor) // IN
{
    if (&textEditor == &visuWidthEditor) {
        int width = textEditor.getText().getIntValue();
        processor.setVisualizationWidth(width);
    } else if (&textEditor == &visuHeightEditor) {
        int height = textEditor.getText().getIntValue();
        processor.setVisualizationHeight(height);
    }
}

void
PatchEditor::GlobalPropertiesComponent::updateVisuSize()
{
    visuWidthEditor.setText(std::to_string(processor.getVisualizationWidth()), false);
    visuHeightEditor.setText(std::to_string(processor.getVisualizationHeight()), false);
}