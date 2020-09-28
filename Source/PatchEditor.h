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
                     public juce::FileBrowserListener
{
public:
    PatchEditor(ShadertoyAudioProcessorEditor *editor);
    ~PatchEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::FileBrowserComponent fileBrowser;
    juce::WildcardFileFilter filter;
    ShadertoyAudioProcessorEditor *editor;
    
    void selectionChanged() override;
    void fileClicked(const juce::File &file, const juce::MouseEvent &e) override;
    void fileDoubleClicked(const juce::File &file) override;
    void browserRootChanged(const juce::File &newRoot) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchEditor)
};
