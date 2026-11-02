#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

GuitarTuneProAudioProcessor::GuitarTuneProAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
GuitarTuneProAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "mode", "Mode",
        juce::StringArray { "AUTO", "MANUAL", "CHROMATIC" }, 0));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "tuning", "Tuning",
        juce::StringArray {
            "E Standard", "Eb Standard", "D Standard", "Drop D",
            "Drop C", "Drop B", "Drop A", "C Standard",
            "Open G", "Open D", "Open E", "DADGAD"
        }, 0));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "string", "String",
        juce::StringArray {
            "6 (lowest)", "5", "4", "3", "2", "1 (highest)"
        }, 0));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "a4", "Reference A4",
        juce::NormalisableRange<float>(432.0f, 445.0f, 0.1f),
        440.0f));

    return { p.begin(), p.end() };
}

void GuitarTuneProAudioProcessor::prepareToPlay(double sr, int)
{
    currentSampleRate = sr;
    analysisBuffer.assign(8192, 0.0f);

    writePosition = 0;
    samplesSinceAnalysis = 0;
    analysisHop = juce::jmax(512, (int)(sr * 0.045));

    stableHz = 0.0f;
    pendingHz = 0.0f;
    pendingCount = 0;

    detectedHz.store(0.0f);
    confidence.store(0.0f);
}

void GuitarTuneProAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int channels = buffer.getNumChannels();
    const int n = buffer.getNumSamples();

    if (analysisBuffer.size() != 8192 || channels == 0)
        return;

    for (int i = 0; i < n; ++i)
    {
        float sample = 0.0f;

        for (int ch = 0; ch < channels; ++ch)
            sample += buffer.getReadPointer(ch)[i];

        sample /= (float)channels;

        analysisBuffer[(size_t)writePosition] = sample;
        writePosition = (writePosition + 1) % (int)analysisBuffer.size();

        if (++samplesSinceAnalysis >= analysisHop)
        {
            samplesSinceAnalysis = 0;
            analyseBuffer();
        }
    }
}

void GuitarTuneProAudioProcessor::analyseBuffer()
{
    constexpr int window = 4096;

    if (analysisBuffer.size() < 8192 || currentSampleRate <= 0.0)
        return;

    std::array<float, window> x {};

    const int start =
        (writePosition - window + (int)analysisBuffer.size())
        % (int)analysisBuffer.size();

    float mean = 0.0f;

    for (int i = 0; i < window; ++i)
        mean += analysisBuffer[(size_t)((start + i) % (int)analysisBuffer.size())];

    mean /= (float)window;

    float rms = 0.0f;

    for (int i = 0; i < window; ++i)
    {
        x[(size_t)i] =
            analysisBuffer[(size_t)((start + i) % (int)analysisBuffer.size())]
            - mean;

        rms += x[(size_t)i] * x[(size_t)i];
    }

    rms = std::sqrt(rms / (float)window);

    if (rms < 0.00035f)
    {
        stableHz = 0.0f;
        pendingHz = 0.0f;
        pendingCount = 0;

        detectedHz.store(0.0f);
        confidence.store(0.0f);
        return;
    }

    const int minLag = juce::jmax(2, (int)(currentSampleRate / 1000.0));
    const int maxLag = juce::jmin(window / 2, (int)(currentSampleRate / 45.0));

    std::array<float, window / 2 + 1> cmnd {};
    cmnd.fill(1.0f);

    double runningSum = 0.0;

    // YIN difference function and cumulative mean normalisation.
    for (int lag = 1; lag <= maxLag; ++lag)
    {
        double difference = 0.0;
        const int count = window - lag;

        for (int i = 0; i < count; ++i)
        {
            const double delta =
                (double)x[(size_t)i] - (double)x[(size_t)(i + lag)];

            difference += delta * delta;
        }

        runningSum += difference;

        cmnd[(size_t)lag] =
            runningSum > 1.0e-12
                ? (float)(difference * lag / runningSum)
                : 1.0f;
    }

    int bestLag = 0;

    // Find the first convincing local minimum.
    for (int lag = minLag + 1; lag < maxLag; ++lag)
    {
        const float previous = cmnd[(size_t)(lag - 1)];
        const float current = cmnd[(size_t)lag];
        const float next = cmnd[(size_t)(lag + 1)];

        if (current < 0.22f &&
            current <= previous &&
            current <= next)
        {
            bestLag = lag;
            break;
        }
    }

    if (bestLag == 0)
    {
        detectedHz.store(stableHz);
        confidence.store(0.0f);
        return;
    }

    // Sub-sample interpolation for better tuning precision.
    float offset = 0.0f;

    const float ym = cmnd[(size_t)(bestLag - 1)];
    const float y0 = cmnd[(size_t)bestLag];
    const float yp = cmnd[(size_t)(bestLag + 1)];

    const float denominator = ym - 2.0f * y0 + yp;

    if (std::abs(denominator) > 1.0e-6f)
        offset = 0.5f * (ym - yp) / denominator;

    offset = juce::jlimit(-0.5f, 0.5f, offset);

    const double period = (double)bestLag + (double)offset;
    const float candidateHz = (float)(currentSampleRate / period);

    const float quality = juce::jlimit(
        0.0f, 1.0f, 1.0f - y0);

    // Reject uncertain or out-of-range measurements.
    if (candidateHz < 45.0f ||
        candidateHz > 1000.0f ||
        quality < 0.72f)
    {
        detectedHz.store(stableHz);
        confidence.store(0.0f);
        return;
    }

    // A small change is treated as normal pitch movement.
    if (stableHz > 0.0f &&
        std::abs(candidateHz - stableHz) / stableHz < 0.035f)
    {
        stableHz = stableHz * 0.65f + candidateHz * 0.35f;

        pendingHz = 0.0f;
        pendingCount = 0;

        detectedHz.store(stableHz);
        confidence.store(quality);
        return;
    }

    // A new pitch must be confirmed by two consistent measurements.
    if (pendingHz > 0.0f &&
        std::abs(candidateHz - pendingHz) / pendingHz < 0.03f)
    {
        ++pendingCount;
        pendingHz = pendingHz * 0.5f + candidateHz * 0.5f;
    }
    else
    {
        pendingHz = candidateHz;
        pendingCount = 1;
    }

    if (pendingCount >= 2)
    {
        stableHz = pendingHz;
        pendingHz = 0.0f;
        pendingCount = 0;

        detectedHz.store(stableHz);
        confidence.store(quality);
    }
    else
    {
        detectedHz.store(stableHz);
        confidence.store(stableHz > 0.0f ? quality : 0.0f);
    }
}

juce::AudioProcessorEditor*
GuitarTuneProAudioProcessor::createEditor()
{
    return new GuitarTuneProAudioProcessorEditor(*this);
}

void GuitarTuneProAudioProcessor::getStateInformation(
    juce::MemoryBlock& dest)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void GuitarTuneProAudioProcessor::setStateInformation(
    const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));

    if (xml != nullptr &&
        xml->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GuitarTuneProAudioProcessor();
}
