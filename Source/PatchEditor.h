/*******************************************************************************

    PatchEditor.h
    Created: 17 Aug 2020 7:16:02pm
    Author:  Austin Borger, aaborger@gmail.com

*******************************************************************************/

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
class PatchEditor : public juce::Component,
                    public ShadertoyAudioProcessor::StateListener
{
public:
    PatchEditor(ShadertoyAudioProcessorEditor &editor,
                ShadertoyAudioProcessor &processor);
    ~PatchEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void processorStateChanged() override;
    void childBoundsChanged(juce::Component *child) override;

private:
    class ShaderListBoxModel : public juce::TableListBoxModel
    {
    public:
        ShaderListBoxModel(juce::TableListBox *box,
                           PatchEditor &patchEditor,
                           ShadertoyAudioProcessor &processor);
        virtual ~ShaderListBoxModel() { }

        int getNumRows() override;
        void paintRowBackground(juce::Graphics &, int rowNumber, int width,
                                int height, bool rowIsSelected) override;
        void paintCell(juce::Graphics &, int rowNumber, int columnId, int width,
                       int height, bool rowIsSelected) override;
        void selectedRowsChanged(int lastRowSelected) override;
        void cellDoubleClicked(int rowNumber, int columnId,
                               const juce::MouseEvent &) override;

        void newRow();
        void deleteSelectedRow();
        void reloadSelectedRow();
        int getSelectedRow();
        
    private:
        juce::Font font { 14.0f };
        juce::TableListBox *box;

        PatchEditor &patchEditor;
        ShadertoyAudioProcessor &processor;
        int selectedRow;
    };

    class ShaderListComponent : public juce::Component,
                                public juce::Button::Listener,
                                public juce::ComponentBoundsConstrainer
    {
    public:
        ShaderListComponent(ShadertoyAudioProcessorEditor &editor,
                            ShadertoyAudioProcessor &processor,
                            PatchEditor &parent);

        void paint(juce::Graphics&) override;
        void resized() override;
        void buttonClicked(juce::Button *) override;
        void checkBounds(juce::Rectangle<int>& bounds,
                         const juce::Rectangle<int>& previousBounds,
                         const juce::Rectangle<int>& limits,
                         bool isStretchingTop,
                         bool isStretchingLeft,
                         bool isStretchingBottom,
                         bool isStretchingRight) override;

        int getSelectedRow();

    private:
        juce::Label shaderListLabel;
        juce::TableListBox shaderListBox;
        juce::TextButton newShaderButton;
        juce::TextButton deleteButton;
        juce::TextButton reloadButton;
        juce::TextButton reloadAllButton;
        juce::ResizableEdgeComponent resizer;

        ShaderListBoxModel shaderListBoxModel;

        ShadertoyAudioProcessorEditor &editor;
        ShadertoyAudioProcessor &processor;
        PatchEditor &parent;

        static constexpr unsigned int BUTTON_WIDTH = 75;
        static constexpr unsigned int BUTTON_HEIGHT = 20;
        static constexpr unsigned int BUTTON_SPACING = 10;
    };

    class ShaderPropertiesComponent : public juce::Component,
                                      public juce::Button::Listener,
                                      public juce::TextEditor::Listener,
                                      public juce::ComboBox::Listener
    {
    public:
        ShaderPropertiesComponent(ShadertoyAudioProcessorEditor &editor,
                                  ShadertoyAudioProcessor &processor,
                                  PatchEditor &parent,
                                  ShaderListComponent &shaderListComponent);

        void paint(juce::Graphics&) override;
        void resized() override;
        void buttonClicked(juce::Button *) override;
        void textEditorTextChanged(juce::TextEditor &) override;
        void comboBoxChanged(juce::ComboBox *comboBoxThatHasChanged) override;

        void greyOutFixedSizeEditors();
        void activateFixedSizeEditors();
        void greyOut();
        void load(int shaderIdx);

    private:
        juce::Label shaderPropertiesLabel;
        juce::ToggleButton fixedSizeButton;
        juce::TextEditor fixedSizeWidthEditor;
        juce::Label fixedSizeWidthLabel;
        juce::TextEditor fixedSizeHeightEditor;
        juce::Label fixedSizeHeightLabel;
        juce::ComboBox destinationBox;
        juce::Label destinationLabel;

        ShadertoyAudioProcessorEditor &editor;
        ShadertoyAudioProcessor &processor;
        PatchEditor &parent;
        ShaderListComponent &shaderListComponent;
    };

    class GlobalPropertiesComponent : public juce::Component,
                                      public juce::TextEditor::Listener
    {
    public:
        GlobalPropertiesComponent(ShadertoyAudioProcessorEditor &editor,
                                  ShadertoyAudioProcessor &processor,
                                  PatchEditor &parent);

        void paint(juce::Graphics&) override;
        void resized() override;
        void textEditorTextChanged(juce::TextEditor &) override;

        void updateVisuSize();

    private:
        juce::Label globalPropertiesLabel;
        juce::TextEditor visuWidthEditor;
        juce::Label visuWidthLabel;
        juce::TextEditor visuHeightEditor;
        juce::Label visuHeightLabel;

        ShadertoyAudioProcessorEditor &editor;
        ShadertoyAudioProcessor &processor;
        PatchEditor &parent;
    };

    void loadTopRightRegion(int shaderIdx);
    void greyOutTopRightRegion();

    ShaderListComponent shaderListComponent;
    ShaderPropertiesComponent shaderPropertiesComponent;
    GlobalPropertiesComponent globalPropertiesComponent;

    ShadertoyAudioProcessorEditor &editor;
    ShadertoyAudioProcessor &processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchEditor)
};
