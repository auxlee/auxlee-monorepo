#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AuxleeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    AuxleeAudioProcessorEditor(AuxleeAudioProcessor&);
    ~AuxleeAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshTrackList();
    void loadSelectedTrack();

    AuxleeAudioProcessor& audioProcessor;

    juce::TextButton recordButton;
    juce::Label apiUrlLabel;
    juce::TextEditor apiUrlEditor;
    juce::Label usernameLabel;
    juce::TextEditor usernameEditor;
    juce::Label passwordLabel;
    juce::TextEditor passwordEditor;
    juce::TextButton connectButton;
    juce::Label statusLabel;
    
    // Track management UI
    juce::Label tracksLabel;
    juce::ComboBox trackSelector;
    juce::TextButton refreshTracksButton;
    juce::TextButton loadTrackButton;
    juce::Array<juce::String> trackIds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuxleeAudioProcessorEditor)
};
