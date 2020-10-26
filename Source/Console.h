/*
  ==============================================================================

    Console.h
    Created: 18 Oct 2020 5:53:04pm
    Author:  Austin Borger, aaborger@gmail.com

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class Console : public juce::Component
{
public:
    Console();
    ~Console() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void logMessage(const juce::String &message);

private:
    juce::TextEditor output;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Console)
};
