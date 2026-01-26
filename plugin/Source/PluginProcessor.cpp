#include "PluginProcessor.h"
#include "PluginEditor.h"

AuxleeAudioProcessor::AuxleeAudioProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    networkClient = std::make_unique<NetworkClient>();
    audioStreamer = std::make_unique<AudioStreamer>(networkClient.get());
}

AuxleeAudioProcessor::~AuxleeAudioProcessor()
{
}

const juce::String AuxleeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AuxleeAudioProcessor::acceptsMidi() const
{
    return false;
}

bool AuxleeAudioProcessor::producesMidi() const
{
    return false;
}

bool AuxleeAudioProcessor::isMidiEffect() const
{
    return false;
}

double AuxleeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AuxleeAudioProcessor::getNumPrograms()
{
    return 1;
}

int AuxleeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AuxleeAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AuxleeAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AuxleeAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void AuxleeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    audioStreamer->prepare(sampleRate, samplesPerBlock);
}

void AuxleeAudioProcessor::releaseResources()
{
    audioStreamer->stop();
}

bool AuxleeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void AuxleeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    // If playing back, mix playback audio into output
    if (isPlaying.load())
    {
        juce::ScopedLock lock(playbackLock);
        
        int samplesToPlay = juce::jmin(buffer.getNumSamples(), 
                                       playbackBuffer.getNumSamples() - playbackPosition.load());
        
        if (samplesToPlay > 0)
        {
            for (int channel = 0; channel < juce::jmin(buffer.getNumChannels(), playbackBuffer.getNumChannels()); ++channel)
            {
                buffer.copyFrom(channel, 0, playbackBuffer, channel, playbackPosition.load(), samplesToPlay);
            }
            
            playbackPosition += samplesToPlay;
            
            // Stop if we've reached the end
            if (playbackPosition >= playbackBuffer.getNumSamples())
            {
                isPlaying = false;
                playbackPosition = 0;
                DBG("Playback finished");
            }
        }
    }

    // Send audio to streamer if recording AND there's actual audio signal
    if (recording)
    {
        // Check if buffer contains any audio (not silence)
        bool hasAudio = false;
        const float silenceThreshold = 0.0001f; // -80dB
        
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const float* channelData = buffer.getReadPointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                if (std::abs(channelData[sample]) > silenceThreshold)
                {
                    hasAudio = true;
                    break;
                }
            }
            if (hasAudio)
                break;
        }
        
        // Only send if there's actual audio signal
        if (hasAudio)
        {
            audioStreamer->addAudioData(buffer);
        }
    }
}

bool AuxleeAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AuxleeAudioProcessor::createEditor()
{
    return new AuxleeAudioProcessorEditor(*this);
}

void AuxleeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    std::unique_ptr<juce::XmlElement> xml(new juce::XmlElement("AuxleeAudioPluginSettings"));
    xml->setAttribute("apiUrl", apiUrl);
    xml->setAttribute("authUsername", authUsername);
    
    copyXmlToBinary(*xml, destData);
}

void AuxleeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName("AuxleeAudioPluginSettings"))
        {
            apiUrl = xmlState->getStringAttribute("apiUrl");
            authUsername = xmlState->getStringAttribute("authUsername");
        }
    }
}

void AuxleeAudioProcessor::setRecording(bool shouldRecord)
{
    recording = shouldRecord;
    
    if (recording)
    {
        // Start a new session
        currentSessionId = networkClient->startSession();
        if (currentSessionId.isNotEmpty())
        {
            audioStreamer->setSessionId(currentSessionId);
            audioStreamer->start();
            DBG("Started recording session: " + currentSessionId);
        }
        else
        {
            DBG("Failed to start session");
            recording = false;
        }
    }
    else
    {
        audioStreamer->stop();
        
        // Finalize the session
        if (currentSessionId.isNotEmpty())
        {
            networkClient->finalizeSession(currentSessionId);
            DBG("Finalized session: " + currentSessionId);
            currentSessionId = "";
        }
    }
}

bool AuxleeAudioProcessor::fetchTracks(juce::Array<juce::String>& trackList)
{
    return networkClient->fetchRecordedTracks(trackList);
}

bool AuxleeAudioProcessor::loadTrack(const juce::String& trackId)
{
    juce::MemoryBlock audioData;
    
    if (!networkClient->downloadTrack(trackId, audioData))
    {
        DBG("Failed to download track");
        return false;
    }
    
    DBG("Downloaded " + juce::String(audioData.getSize()) + " bytes");
    
    // Keep the memory block alive while parsing
    auto inputStream = std::make_unique<juce::MemoryInputStream>(audioData, false);
    
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatReader> reader(wavFormat.createReaderFor(inputStream.get(), true));
    
    if (reader == nullptr)
    {
        DBG("Failed to parse WAV data");
        return false;
    }
    
    // Release ownership so reader can manage the stream
    inputStream.release();
    
    DBG("Parsed WAV: " + juce::String(reader->lengthInSamples) + " samples, " + 
        juce::String(reader->numChannels) + " channels, " + 
        juce::String(reader->sampleRate) + " Hz");
    
    // Create temporary buffer and read audio
    juce::AudioBuffer<float> tempBuffer(static_cast<int>(reader->numChannels),
                                        static_cast<int>(reader->lengthInSamples));
    
    if (!reader->read(&tempBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true))
    {
        DBG("Failed to read audio data");
        return false;
    }
    
    // Safely swap the buffer on the audio thread
    {
        juce::ScopedLock lock(playbackLock);
        
        // Stop current playback
        isPlaying = false;
        playbackPosition = 0;
        
        // Swap in new buffer
        playbackBuffer = std::move(tempBuffer);
        
        // Start playback
        isPlaying = true;
    }
    
    DBG("Track loaded and playing");
    return true;
}

void AuxleeAudioProcessor::setApiUrl(const juce::String& url)
{
    apiUrl = url;
    networkClient->setApiUrl(url);
}

void AuxleeAudioProcessor::setAuthentication(const juce::String& username, const juce::String& password)
{
    authUsername = username;
    authPassword = password;
    networkClient->setAuthentication(username, password);
}

bool AuxleeAudioProcessor::testConnection()
{
    return networkClient->testConnection();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AuxleeAudioProcessor();
}
