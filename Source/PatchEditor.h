/*
  ==============================================================================

    PatchEditor.h
    Created: 17 Aug 2020 7:16:02pm
    Author:  Austin Borger, aaborger@gmail.com

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ShadertoyAudioProcessorEditor;

/*
 * PatchEditor
 *    Describes the GUI for editing the visualization patch.
 *    Provides controls for adding/deleting fragment shaders
 *    and modifying shader-specific / global properties.
 */
class PatchEditor  : public juce::Component,
                     public juce::Button::Listener,
                     public juce::TextEditor::Listener,
                     public ShadertoyAudioProcessor::StateListener
{
public:
    PatchEditor(ShadertoyAudioProcessorEditor &editor,
                ShadertoyAudioProcessor &processor);
    ~PatchEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button *) override;
    void textEditorTextChanged(juce::TextEditor &) override;
    void processorStateChanged() override;

private:
    class ShaderListBoxModel : public juce::TableListBoxModel
    {
    public:
        ShaderListBoxModel(juce::TableListBox *box,
                           PatchEditor &parent,
                           ShadertoyAudioProcessor &processor);
        virtual ~ShaderListBoxModel() { }

        int getNumRows() override;
        void paintRowBackground(juce::Graphics &, int rowNumber, int width,
                                int height, bool rowIsSelected) override;
        void paintCell(juce::Graphics &, int rowNumber, int columnId, int width,
                       int height, bool rowIsSelected) override;
        void selectedRowsChanged(int lastRowSelected) override;
        void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent &) override;

        void newRow();
        void deleteSelectedRow();
        void reloadSelectedRow();
        int getSelectedRow();
        
    private:
        juce::Font font { 14.0f };
        juce::TableListBox *box;

        PatchEditor &parent;
        ShadertoyAudioProcessor &processor;
        int selectedRow;
    };
    
    void greyOutFixedSizeEditors();
    void activateFixedSizeEditors();
    void greyOutTopRightRegion();
    void loadTopRightRegion(int shaderIdx);

    juce::Label shaderListLabel;
    juce::TableListBox shaderListBox;
    juce::TextButton newShaderButton;
    juce::TextButton deleteButton;
    juce::TextButton reloadButton;

    juce::Label shaderPropertiesLabel;
    juce::ToggleButton fixedSizeButton;
    juce::TextEditor fixedSizeWidthEditor;
    juce::Label fixedSizeWidthLabel;
    juce::TextEditor fixedSizeHeightEditor;
    juce::Label fixedSizeHeightLabel;
    juce::ComboBox destinationBox;
    juce::Label destinationLabel;

    juce::Label globalPropertiesLabel;
    juce::TextEditor visuWidthEditor;
    juce::Label visuWidthLabel;
    juce::TextEditor visuHeightEditor;
    juce::Label visuHeightLabel;
    
    ShaderListBoxModel shaderListBoxModel;
    ShadertoyAudioProcessorEditor &editor;
    ShadertoyAudioProcessor &processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchEditor)
};
