#pragma once

#include <JuceHeader.h>

class NetworkClient
{
public:
    NetworkClient();
    ~NetworkClient();

    void setApiUrl(const juce::String& url);
    void setAuthentication(const juce::String& username, const juce::String& password);
    
    bool testConnection();
    juce::String startSession();
    bool finalizeSession(const juce::String& sessionId);
    bool sendAudioChunk(const juce::MemoryBlock& audioData, const juce::String& sessionId);
    bool fetchRecordedTracks(juce::Array<juce::String>& trackList);
    bool downloadTrack(const juce::String& trackId, juce::MemoryBlock& audioData);

private:
    juce::String getAuthHeader() const;
    juce::String formatHeaders() const;
    
    juce::String apiUrl;
    juce::String username;
    juce::String password;
    std::unique_ptr<juce::URL::DownloadTask> currentTask;
};
