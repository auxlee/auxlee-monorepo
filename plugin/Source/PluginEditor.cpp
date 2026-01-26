#include "PluginProcessor.h"
#include "PluginEditor.h"

AuxleeAudioProcessorEditor::AuxleeAudioProcessorEditor(AuxleeAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(400, 550);

    // API URL
    apiUrlLabel.setText("API URL:", juce::dontSendNotification);
    apiUrlLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(apiUrlLabel);

    apiUrlEditor.setText("http://localhost:8000");
    addAndMakeVisible(apiUrlEditor);

    // Username
    usernameLabel.setText("Username:", juce::dontSendNotification);
    usernameLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(usernameLabel);

    usernameEditor.setText("admin");
    addAndMakeVisible(usernameEditor);

    // Password
    passwordLabel.setText("Password:", juce::dontSendNotification);
    passwordLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(passwordLabel);

    passwordEditor.setText("");
    passwordEditor.setPasswordCharacter('*');
    addAndMakeVisible(passwordEditor);

    // Connect button
    connectButton.setButtonText("Connect");
    connectButton.onClick = [this]
    {
        // Change button text immediately to prove onClick is firing
        connectButton.setButtonText("CLICKED!");
        
        auto url = apiUrlEditor.getText();
        auto username = usernameEditor.getText();
        auto password = passwordEditor.getText();
        
        // Set the credentials in the processor
        audioProcessor.setApiUrl(url);
        audioProcessor.setAuthentication(username, password);
        
        // Update status with forced notification
        juce::MessageManager::callAsync([this, url]()
        {
            statusLabel.setText("✓ CONNECTED: " + url, juce::sendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
            statusLabel.setColour(juce::Label::backgroundColourId, juce::Colours::green);
            
            // Hide login form
            apiUrlLabel.setVisible(false);
            apiUrlEditor.setVisible(false);
            usernameLabel.setVisible(false);
            usernameEditor.setVisible(false);
            passwordLabel.setVisible(false);
            passwordEditor.setVisible(false);
            connectButton.setVisible(false);
            
            // Show track management UI
            tracksLabel.setVisible(true);
            trackSelector.setVisible(true);
            refreshTracksButton.setVisible(true);
            loadTrackButton.setVisible(true);
            
            // Load initial track list
            refreshTrackList();
        });
    };
    addAndMakeVisible(connectButton);

    // Record button
    recordButton.setButtonText("Start Recording");
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    recordButton.onClick = [this]
    {
        DBG("=== Record button clicked ===");
        
        bool currentlyRecording = audioProcessor.isRecording();
        audioProcessor.setRecording(!currentlyRecording);
        
        if (!currentlyRecording)
        {
            recordButton.setButtonText("Stop Recording");
            recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
            statusLabel.setText("🔴 RECORDING...", juce::sendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
            statusLabel.setColour(juce::Label::backgroundColourId, juce::Colours::red);
            DBG("Started recording");
        }
        else
        {
            recordButton.setButtonText("Start Recording");
            recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            statusLabel.setText("⏹ Stopped", juce::sendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
            statusLabel.setColour(juce::Label::backgroundColourId, juce::Colours::orange);
            DBG("Stopped recording");
        }
        
        recordButton.repaint();
        statusLabel.repaint();
    };
    addAndMakeVisible(recordButton);

    // Status label
    statusLabel.setText("Not connected", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    statusLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
    statusLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(statusLabel);

    // Track management UI (initially hidden)
    tracksLabel.setText("Available Tracks:", juce::dontSendNotification);
    tracksLabel.setJustificationType(juce::Justification::left);
    tracksLabel.setVisible(false);
    addAndMakeVisible(tracksLabel);
    
    trackSelector.setTextWhenNothingSelected("No tracks available");
    trackSelector.setVisible(false);
    addAndMakeVisible(trackSelector);
    
    refreshTracksButton.setButtonText("Refresh");
    refreshTracksButton.onClick = [this] { refreshTrackList(); };
    refreshTracksButton.setVisible(false);
    addAndMakeVisible(refreshTracksButton);
    
    loadTrackButton.setButtonText("Load Track");
    loadTrackButton.onClick = [this] { loadSelectedTrack(); };
    loadTrackButton.setEnabled(false);
    loadTrackButton.setVisible(false);
    addAndMakeVisible(loadTrackButton);

    startTimerHz(30);
}

AuxleeAudioProcessorEditor::~AuxleeAudioProcessorEditor()
{
}

void AuxleeAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawFittedText("Auxlee Audio Recorder", getLocalBounds().removeFromTop(40), 
                     juce::Justification::centred, 1);
}

void AuxleeAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(40); // Title space

    apiUrlLabel.setBounds(bounds.removeFromTop(25).removeFromLeft(100));
    apiUrlEditor.setBounds(bounds.removeFromTop(25).reduced(5, 0));
    bounds.removeFromTop(10);

    usernameLabel.setBounds(bounds.removeFromTop(25).removeFromLeft(100));
    usernameEditor.setBounds(bounds.removeFromTop(25).reduced(5, 0));
    bounds.removeFromTop(10);

    passwordLabel.setBounds(bounds.removeFromTop(25).removeFromLeft(100));
    passwordEditor.setBounds(bounds.removeFromTop(25).reduced(5, 0));
    bounds.removeFromTop(10);

    connectButton.setBounds(bounds.removeFromTop(30).reduced(80, 0));
    bounds.removeFromTop(15);

    recordButton.setBounds(bounds.removeFromTop(40).reduced(60, 0));
    bounds.removeFromTop(20);

    statusLabel.setBounds(bounds.removeFromTop(50));
    bounds.removeFromTop(15);
    
    // Track management UI
    tracksLabel.setBounds(bounds.removeFromTop(25));
    
    auto trackRow = bounds.removeFromTop(30);
    trackSelector.setBounds(trackRow.removeFromLeft(220));
    trackRow.removeFromLeft(10);
    refreshTracksButton.setBounds(trackRow.removeFromLeft(70));
    
    bounds.removeFromTop(10);
    loadTrackButton.setBounds(bounds.removeFromTop(35).reduced(80, 0));
}

void AuxleeAudioProcessorEditor::timerCallback()
{
    // Update UI based on recording state
}

void AuxleeAudioProcessorEditor::refreshTrackList()
{
    trackSelector.clear();
    trackIds.clear();
    
    juce::Array<juce::String> trackList;
    if (audioProcessor.fetchTracks(trackList))
    {
        for (int i = 0; i < trackList.size(); ++i)
        {
            trackSelector.addItem("Track " + juce::String(i + 1), i + 1);
            trackIds.add(trackList[i]);
        }
        
        loadTrackButton.setEnabled(trackList.size() > 0);
        
        if (trackList.size() > 0)
        {
            trackSelector.setSelectedId(1);
            statusLabel.setText("✓ Found " + juce::String(trackList.size()) + " track(s)", juce::sendNotification);
            statusLabel.setColour(juce::Label::backgroundColourId, juce::Colours::green);
        }
        else
        {
            statusLabel.setText("No tracks available", juce::sendNotification);
            statusLabel.setColour(juce::Label::backgroundColourId, juce::Colours::orange);
        }
    }
    else
    {
        statusLabel.setText("Failed to fetch tracks", juce::sendNotification);
        statusLabel.setColour(juce::Label::backgroundColourId, juce::Colours::red);
    }
}

void AuxleeAudioProcessorEditor::loadSelectedTrack()
{
    int selectedIndex = trackSelector.getSelectedItemIndex();
    if (selectedIndex >= 0 && selectedIndex < trackIds.size())
    {
        statusLabel.setText("Loading track...", juce::sendNotification);
        statusLabel.setColour(juce::Label::backgroundColourId, juce::Colours::blue);
        
        if (audioProcessor.loadTrack(trackIds[selectedIndex]))
        {
            statusLabel.setText("✓ Track loaded - Playing!", juce::sendNotification);
            statusLabel.setColour(juce::Label::backgroundColourId, juce::Colours::green);
        }
        else
        {
            statusLabel.setText("Failed to load track", juce::sendNotification);
            statusLabel.setColour(juce::Label::backgroundColourId, juce::Colours::red);
        }
    }
}
