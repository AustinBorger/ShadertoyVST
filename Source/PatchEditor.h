/*
  ==============================================================================

    PatchEditor.h
    Created: 17 Aug 2020 7:16:02pm
    Author:  Austin

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ShadertoyAudioProcessorEditor;

//==============================================================================
/*
*/
class PatchEditor  : public juce::Component,
                     public juce::Button::Listener
{
public:
    PatchEditor(ShadertoyAudioProcessorEditor *editor);
    ~PatchEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button *) override;
    
    void updateShaders();

private:
    class ShaderListBoxModel : public juce::TableListBoxModel
    {
    public:
        ShaderListBoxModel(juce::TableListBox *box,
                           PatchEditor *parent);
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
        
    private:
        juce::Font font { 14.0f };
        juce::TableListBox *box;

        PatchEditor *parent;
        std::vector<juce::String> shaderLocations;
        int selectedRow;
    };

    juce::TableListBox shaderListBox;
    juce::TextButton newShaderButton;
    juce::TextButton deleteButton;
    
    ShaderListBoxModel shaderListBoxModel;
    ShadertoyAudioProcessorEditor *editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchEditor)
};
