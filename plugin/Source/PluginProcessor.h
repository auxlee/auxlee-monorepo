#pragma once

#include <JuceHeader.h>
#include "AudioStreamer.h"
#include "NetworkClient.h"

class AuxleeAudioProcessor : public juce::AudioProcessor
{
public:
    AuxleeAudioProcessor();
    ~AuxleeAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Custom methods
    void setRecording(bool shouldRecord);
    bool isRecording() const { return recording; }
    void setApiUrl(const juce::String& url);
    void setAuthentication(const juce::String& username, const juce::String& password);
    bool testConnection();
    bool fetchTracks(juce::Array<juce::String>& trackList);
    bool loadTrack(const juce::String& trackId);

private:
    std::unique_ptr<AudioStreamer> audioStreamer;
    std::unique_ptr<NetworkClient> networkClient;
    bool recording = false;
    juce::String apiUrl;
    juce::String authUsername;
    juce::String authPassword;
    juce::String currentSessionId;  // Track current recording session
    
    // Playback buffer
    juce::AudioBuffer<float> playbackBuffer;
    std::atomic<int> playbackPosition{ 0 };
    std::atomic<bool> isPlaying{ false };
    juce::CriticalSection playbackLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuxleeAudioProcessor)
};
