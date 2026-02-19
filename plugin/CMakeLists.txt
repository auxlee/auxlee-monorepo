cmake_minimum_required(VERSION 3.15)

project(AuxleeAudioPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE setup - assumes JUCE is installed via CMake or as a submodule
# You'll need to clone JUCE: git clone https://github.com/juce-framework/JUCE.git
add_subdirectory(JUCE)

juce_add_plugin(AuxleeAudioPlugin
    COMPANY_NAME "Auxlee"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Auxl
    PLUGIN_CODE Axlp
    FORMATS AU VST3 CLAP Standalone
    PRODUCT_NAME "Auxlee Audio Recorder"
)

juce_generate_juce_header(AuxleeAudioPlugin)

target_sources(AuxleeAudioPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/AudioStreamer.cpp
        Source/NetworkClient.cpp
)

target_compile_definitions(AuxleeAudioPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(AuxleeAudioPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
