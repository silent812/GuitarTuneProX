#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class GuitarTuneProAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit GuitarTuneProAudioProcessorEditor(GuitarTuneProAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& event) override;

    GuitarTuneProAudioProcessor& processor;

    juce::ComboBox modeBox, tuningBox, stringBox;
    juce::Slider referenceSlider;

    juce::Label title, subtitle, noteLabel, centsLabel, hintLabel, hzLabel;
    juce::Label modeLabel, tuningLabel, stringLabel, refLabel;

    float displayCents = 0.0f;
    float displayHz = 0.0f;
    bool smoothingInitialized = false;
    bool signalPresent = false;
    bool tuningOk = false;
    int titleClickCount = 0;
    juce::int64 lastTitleClickMs = 0;
    juce::int64 secretUntilMs = 0;
    float animationPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarTuneProAudioProcessorEditor)
};
