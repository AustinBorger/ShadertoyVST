/*
  ==============================================================================

    PatchEditor.h
    Created: 17 Aug 2020 7:16:02pm
    Author:  Austin

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class PatchEditor  : public juce::Component
{
public:
    PatchEditor();
    ~PatchEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::FileBrowserComponent fileBrowser;
    juce::WildcardFileFilter filter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchEditor)
};
