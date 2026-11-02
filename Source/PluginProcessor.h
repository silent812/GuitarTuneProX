#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

class GuitarTuneProAudioProcessor : public juce::AudioProcessor
{
public:
    GuitarTuneProAudioProcessor();
    ~GuitarTuneProAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Guitar Tune Pro X"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    std::atomic<float> detectedHz { 0.0f };
    std::atomic<float> confidence { 0.0f };
    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
private:
    double currentSampleRate = 44100.0;
    std::vector<float> analysisBuffer;
    int writePosition = 0, samplesSinceAnalysis = 0, analysisHop = 2048;

    // Pitch stabilisation state
    float stableHz = 0.0f;
    float pendingHz = 0.0f;
    int pendingCount = 0;
    void analyseBuffer();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarTuneProAudioProcessor)
};


