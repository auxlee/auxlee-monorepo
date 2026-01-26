#include "NetworkClient.h"

NetworkClient::NetworkClient()
{
}

NetworkClient::~NetworkClient()
{
}

void NetworkClient::setApiUrl(const juce::String& url)
{
    apiUrl = url;
}

void NetworkClient::setAuthentication(const juce::String& user, const juce::String& pass)
{
    username = user;
    password = pass;
}

juce::String NetworkClient::getAuthHeader() const
{
    juce::String credentials = username + ":" + password;
    juce::String encoded = juce::Base64::toBase64(credentials);
    return "Basic " + encoded;
}

juce::String NetworkClient::formatHeaders() const
{
    juce::String headers;
    headers << "Authorization: " << getAuthHeader() << "\r\n";
    headers << "Content-Type: audio/wav\r\n";
    return headers;
}

bool NetworkClient::testConnection()
{
    if (apiUrl.isEmpty())
        return false;

    juce::URL url(apiUrl + "/");
    
    juce::String headers;
    headers << "Authorization: " << getAuthHeader() << "\r\n";

    std::unique_ptr<juce::InputStream> response(url.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders(headers)
            .withConnectionTimeoutMs(3000)
    ));

    if (response != nullptr)
    {
        juce::String responseText = response->readEntireStreamAsString();
        return responseText.isNotEmpty();
    }

    return false;
}

juce::String NetworkClient::startSession()
{
    if (apiUrl.isEmpty())
        return "";

    juce::URL url(apiUrl + "/api/start-session");
    
    juce::String headers;
    headers << "Authorization: " << getAuthHeader() << "\r\n";

    std::unique_ptr<juce::InputStream> response(url.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders(headers)
            .withConnectionTimeoutMs(5000)
            .withHttpRequestCmd("POST")
    ));

    if (response != nullptr)
    {
        juce::String responseText = response->readEntireStreamAsString();
        DBG("Start session response: " + responseText);
        
        // Parse JSON to get session_id
        juce::var json;
        juce::Result result = juce::JSON::parse(responseText, json);
        
        if (result.wasOk() && json.hasProperty("session_id"))
        {
            return json["session_id"].toString();
        }
    }

    return "";
}

bool NetworkClient::finalizeSession(const juce::String& sessionId)
{
    if (apiUrl.isEmpty() || sessionId.isEmpty())
        return false;

    juce::URL url(apiUrl + "/api/finalize-session");
    url = url.withParameter("session_id", sessionId);
    
    juce::String headers;
    headers << "Authorization: " << getAuthHeader() << "\r\n";

    std::unique_ptr<juce::InputStream> response(url.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders(headers)
            .withConnectionTimeoutMs(5000)
            .withHttpRequestCmd("POST")
    ));

    if (response != nullptr)
    {
        juce::String responseText = response->readEntireStreamAsString();
        DBG("Finalize session response: " + responseText);
        return true;
    }

    return false;
}

bool NetworkClient::sendAudioChunk(const juce::MemoryBlock& audioData, const juce::String& sessionId)
{
    if (apiUrl.isEmpty())
        return false;

    // Create multipart boundary
    juce::String boundary = "----JUCEAudioBoundary" + juce::String(juce::Random::getSystemRandom().nextInt64());
    
    // Build multipart form data
    juce::MemoryBlock formData;
    juce::MemoryOutputStream stream(formData, false);
    
    // Write form field
    stream << "--" << boundary << "\r\n";
    stream << "Content-Disposition: form-data; name=\"file\"; filename=\"chunk.wav\"\r\n";
    stream << "Content-Type: audio/wav\r\n\r\n";
    
    // Write audio data
    stream.write(audioData.getData(), audioData.getSize());
    
    // Write closing boundary
    stream << "\r\n--" << boundary << "--\r\n";
    
    // Prepare headers
    juce::String headers;
    headers << "Authorization: " << getAuthHeader() << "\r\n";
    headers << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";
    
    // Send POST request with body and session_id parameter
    juce::URL url(apiUrl + "/api/upload-chunk");
    if (sessionId.isNotEmpty())
        url = url.withParameter("session_id", sessionId);
    url = url.withPOSTData(formData);
    
    std::unique_ptr<juce::InputStream> response(url.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders(headers)
            .withConnectionTimeoutMs(5000)
            .withNumRedirectsToFollow(0)
    ));

    if (response != nullptr)
    {
        juce::String responseText = response->readEntireStreamAsString();
        DBG("Upload response: " + responseText);
        return true;
    }

    DBG("Failed to upload chunk");
    return false;
}

bool NetworkClient::fetchRecordedTracks(juce::Array<juce::String>& trackList)
{
    if (apiUrl.isEmpty())
        return false;

    juce::URL url(apiUrl + "/api/tracks");
    
    juce::String headers;
    headers << "Authorization: " << getAuthHeader() << "\r\n";

    std::unique_ptr<juce::InputStream> response(url.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders(headers)
            .withConnectionTimeoutMs(5000)
    ));

    if (response != nullptr)
    {
        juce::String responseText = response->readEntireStreamAsString();
        
        // Parse JSON response
        juce::var json;
        juce::Result result = juce::JSON::parse(responseText, json);
        
        if (result.wasOk() && json.isArray())
        {
            for (auto& track : *json.getArray())
            {
                if (track.hasProperty("id"))
                    trackList.add(track["id"].toString());
            }
            return true;
        }
    }

    return false;
}

bool NetworkClient::downloadTrack(const juce::String& trackId, juce::MemoryBlock& audioData)
{
    if (apiUrl.isEmpty())
        return false;

    juce::URL url(apiUrl + "/api/download/" + trackId);
    
    juce::String headers;
    headers << "Authorization: " << getAuthHeader() << "\r\n";

    std::unique_ptr<juce::InputStream> response(url.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders(headers)
            .withConnectionTimeoutMs(10000)
    ));

    if (response != nullptr)
    {
        audioData.reset();
        
        char buffer[8192];
        while (!response->isExhausted())
        {
            auto bytesRead = response->read(buffer, sizeof(buffer));
            if (bytesRead > 0)
                audioData.append(buffer, static_cast<size_t>(bytesRead));
        }
        
        return audioData.getSize() > 0;
    }

    return false;
}
