#pragma once

#include <JuceHeader.h>
#include "NetworkClient.h"

class AudioStreamer
{
public:
    AudioStreamer(NetworkClient* client);
    ~AudioStreamer();

    void prepare(double sampleRate, int blockSize);
    void start();
    void stop();
    void addAudioData(const juce::AudioBuffer<float>& buffer);
    void setSessionId(const juce::String& sessionId);

private:
    void sendChunk();

    NetworkClient* networkClient;
    juce::AudioBuffer<float> bufferQueue;
    juce::CriticalSection bufferLock;
    
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    int chunkSize = 44100 * 2; // 2 seconds worth of samples
    int currentPosition = 0;
    juce::String currentSessionId;
    
    std::atomic<bool> isStreaming{ false };
    std::unique_ptr<juce::Thread> streamThread;
};
