/*
  ==============================================================================

    Console.cpp
    Created: 18 Oct 2020 5:53:04pm
    Author:  Austin Borger, aaborger@gmail.com

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Console.h"

//==============================================================================
Console::Console()
{
    addAndMakeVisible(output);
    output.setMultiLine(true);
    output.setReturnKeyStartsNewLine(true);
    output.setReadOnly(true);
    output.setScrollbarsShown(true);
    output.setCaretVisible(false);
    output.setPopupMenuEnabled(true);
    output.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x32ffffff));
    output.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x1c000000));
    output.setColour(juce::TextEditor::shadowColourId, juce::Colour(0x16000000));
}

Console::~Console()
{
}

void Console::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void Console::resized()
{
    output.setBounds(0, 0, getWidth(), getHeight());
}

void Console::logMessage(const juce::String &message)
{
    output.moveCaretToEnd();
    output.insertTextAtCaret(message + juce::newLine);
}