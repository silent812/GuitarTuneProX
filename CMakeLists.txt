cmake_minimum_required(VERSION 3.22)

project(GuitarTuneProX VERSION 2.0.0)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
)

FetchContent_MakeAvailable(JUCE)

juce_add_plugin(GuitarTuneProX
    COMPANY_NAME "SILENTBROKEN"
    PLUGIN_MANUFACTURER_CODE SlBr
    PLUGIN_CODE GtX1
    FORMATS VST3
    PRODUCT_NAME "Guitar Tune Pro X"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(GuitarTuneProX PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_features(GuitarTuneProX PRIVATE cxx_std_17)

target_link_libraries(GuitarTuneProX PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)

target_compile_definitions(GuitarTuneProX PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

juce_generate_juce_header(GuitarTuneProX)
