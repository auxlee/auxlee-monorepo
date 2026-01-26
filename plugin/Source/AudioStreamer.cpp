#include "AudioStreamer.h"

AudioStreamer::AudioStreamer(NetworkClient* client)
    : networkClient(client)
{
}

AudioStreamer::~AudioStreamer()
{
    stop();
}

void AudioStreamer::prepare(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    currentBlockSize = blockSize;
    chunkSize = static_cast<int>(sampleRate * 2.0); // 2 seconds of audio
}

void AudioStreamer::start()
{
    juce::ScopedLock lock(bufferLock);
    isStreaming = true;
    bufferQueue.setSize(2, chunkSize * 10); // Buffer for up to 20 seconds
    bufferQueue.clear();
    currentPosition = 0;
}

void AudioStreamer::stop()
{
    if (isStreaming)
    {
        // Send remaining audio
        if (currentPosition > 0)
        {
            sendChunk();
        }
        isStreaming = false;
    }
}

void AudioStreamer::addAudioData(const juce::AudioBuffer<float>& buffer)
{
    if (!isStreaming)
        return;

    juce::ScopedLock lock(bufferLock);

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Ensure buffer has space
    if (currentPosition + numSamples > bufferQueue.getNumSamples())
    {
        // Send current chunk before overflow
        sendChunk();
    }

    // Copy audio data to buffer
    for (int channel = 0; channel < juce::jmin(numChannels, bufferQueue.getNumChannels()); ++channel)
    {
        bufferQueue.copyFrom(channel, currentPosition, buffer, channel, 0, numSamples);
    }

    currentPosition += numSamples;

    // Send chunk if we've accumulated enough data
    if (currentPosition >= chunkSize)
    {
        sendChunk();
    }
}

void AudioStreamer::sendChunk()
{
    if (currentPosition == 0 || networkClient == nullptr)
        return;

    // Prepare audio data for sending
    juce::MemoryBlock audioData;
    
    // Write WAV header information
    struct WavHeader
    {
        char riff[4] = {'R', 'I', 'F', 'F'};
        uint32_t fileSize;
        char wave[4] = {'W', 'A', 'V', 'E'};
        char fmt[4] = {'f', 'm', 't', ' '};
        uint32_t fmtSize = 16;
        uint16_t audioFormat = 1; // PCM
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample = 16;
        char data[4] = {'d', 'a', 't', 'a'};
        uint32_t dataSize;
    };

    WavHeader header;
    header.numChannels = static_cast<uint16_t>(bufferQueue.getNumChannels());
    header.sampleRate = static_cast<uint32_t>(currentSampleRate);
    header.byteRate = header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
    header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
    header.dataSize = currentPosition * header.numChannels * (header.bitsPerSample / 8);
    header.fileSize = 36 + header.dataSize;

    audioData.append(&header, sizeof(WavHeader));

    // Convert float samples to 16-bit PCM
    for (int i = 0; i < currentPosition; ++i)
    {
        for (int channel = 0; channel < bufferQueue.getNumChannels(); ++channel)
        {
            float sample = bufferQueue.getSample(channel, i);
            int16_t pcmSample = static_cast<int16_t>(juce::jlimit(-1.0f, 1.0f, sample) * 32767.0f);
            audioData.append(&pcmSample, sizeof(int16_t));
        }
    }

    // Send to network client
    networkClient->sendAudioChunk(audioData, currentSessionId);

    // Reset position
    currentPosition = 0;
}

void AudioStreamer::setSessionId(const juce::String& sessionId)
{
    currentSessionId = sessionId;
}
