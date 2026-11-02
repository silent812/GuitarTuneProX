#include "PluginEditor.h"
#include <cmath>
#include <array>

namespace
{
const std::array<std::array<int, 6>, 12> tunings {{
    {{40,45,50,55,59,64}},
    {{39,44,49,54,58,63}},
    {{38,43,48,53,57,62}},
    {{38,45,50,55,59,64}},
    {{36,43,48,53,57,62}},
    {{35,42,47,52,56,61}},
    {{33,40,45,50,54,59}},
    {{36,41,46,51,55,60}},
    {{38,43,50,55,59,62}},
    {{38,45,50,54,57,62}},
    {{40,47,52,56,59,64}},
    {{38,45,50,55,57,62}}
}};

const char* names[] = {
    "C","C#","D","D#","E","F",
    "F#","G","G#","A","A#","B"
};

juce::String noteName(int midi)
{
    return juce::String(names[(midi % 12 + 12) % 12])
         + juce::String(midi / 12 - 1);
}
}

GuitarTuneProAudioProcessorEditor::GuitarTuneProAudioProcessorEditor(
    GuitarTuneProAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1000, 700);
    setResizable(false, false);

    auto setupLabel = [](juce::Label& label,
                         const juce::String& text,
                         float size,
                         juce::Colour colour)
    {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(size, juce::Font::bold));
        label.setColour(juce::Label::textColourId, colour);
        label.setJustificationType(juce::Justification::centred);
    };

    const auto white = juce::Colour(0xffe8f1f8);
    const auto muted = juce::Colour(0xff8498aa);
    const auto cyan = juce::Colour(0xffff3b30);

    setupLabel(title, "GUITAR TUNE PRO X", 27.0f, white);
    title.setInterceptsMouseClicks(false, false);
    setupLabel(subtitle, "DIGITAL EDITION  |  REAL-TIME GUITAR TUNER", 12.0f, cyan);
    setupLabel(noteLabel, "--", 86.0f, muted);
    setupLabel(centsLabel, "-- cents", 25.0f, muted);
    setupLabel(hintLabel, "PLAY ONE OPEN STRING", 15.0f, muted);
    setupLabel(hzLabel, "Waiting for audio signal", 14.0f, muted);

    setupLabel(modeLabel, "MODE", 11.0f, cyan);
    setupLabel(tuningLabel, "TUNING", 11.0f, cyan);
    setupLabel(stringLabel, "STRING", 11.0f, cyan);
    setupLabel(refLabel, "A4 REFERENCE", 11.0f, cyan);

    for (auto* label : {
        &title, &subtitle, &noteLabel, &centsLabel,
        &hintLabel, &hzLabel, &modeLabel, &tuningLabel,
        &stringLabel, &refLabel
    })
        addAndMakeVisible(*label);

    // These labels are drawn manually in paint().
    // Hide their JUCE components to prevent duplicate text.
    noteLabel.setVisible(false);
    centsLabel.setVisible(false);
    hintLabel.setVisible(false);
    hzLabel.setVisible(false);

    modeBox.addItemList({"AUTO", "MANUAL", "CHROMATIC"}, 1);
    tuningBox.addItemList({
        "E Standard", "Eb Standard", "D Standard", "Drop D",
        "Drop C", "Drop B", "Drop A", "C Standard",
        "Open G", "Open D", "Open E", "DADGAD"
    }, 1);
    stringBox.addItemList({
        "6 - Low", "5", "4", "3", "2", "1 - High"
    }, 1);

    for (auto* box : {&modeBox, &tuningBox, &stringBox})
    {
        box->setColour(juce::ComboBox::backgroundColourId,
                       juce::Colour(0xff17232e));
        box->setColour(juce::ComboBox::textColourId, white);
        box->setColour(juce::ComboBox::outlineColourId,
                       juce::Colour(0xff315064));
        box->setColour(juce::ComboBox::arrowColourId, cyan);
        addAndMakeVisible(*box);
    }

    modeBox.setSelectedItemIndex(
        (int)processor.parameters.getRawParameterValue("mode")->load(),
        juce::dontSendNotification);

    tuningBox.setSelectedItemIndex(
        (int)processor.parameters.getRawParameterValue("tuning")->load(),
        juce::dontSendNotification);

    stringBox.setSelectedItemIndex(
        (int)processor.parameters.getRawParameterValue("string")->load(),
        juce::dontSendNotification);

    modeBox.onChange = [this]
    {
        processor.parameters.getParameterAsValue("mode") =
            modeBox.getSelectedId() - 1;
    };

    tuningBox.onChange = [this]
    {
        processor.parameters.getParameterAsValue("tuning") =
            tuningBox.getSelectedId() - 1;
    };

    stringBox.onChange = [this]
    {
        processor.parameters.getParameterAsValue("string") =
            stringBox.getSelectedId() - 1;
    };

    referenceSlider.setSliderStyle(
        juce::Slider::LinearHorizontal);

    referenceSlider.setTextBoxStyle(
        juce::Slider::TextBoxRight, false, 75, 25);

    referenceSlider.setRange(432.0, 445.0, 0.1);

    referenceSlider.setValue(
        processor.parameters.getRawParameterValue("a4")->load(),
        juce::dontSendNotification);

    referenceSlider.setTextValueSuffix(" Hz");

    referenceSlider.setColour(
        juce::Slider::trackColourId, cyan);

    referenceSlider.setColour(
        juce::Slider::thumbColourId,
        juce::Colour(0xffff8a30));

    referenceSlider.setColour(
        juce::Slider::textBoxTextColourId, white);

    referenceSlider.setColour(
        juce::Slider::textBoxBackgroundColourId,
        juce::Colour(0xff17232e));

    referenceSlider.onValueChange = [this]
    {
        processor.parameters.getParameterAsValue("a4") =
            referenceSlider.getValue();
    };

    addAndMakeVisible(referenceSlider);

    startTimerHz(20);
}

void GuitarTuneProAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (!title.getBounds().contains(event.getPosition()))
        return;

    const juce::int64 now = (juce::int64)juce::Time::getMillisecondCounterHiRes();

    if (now - lastTitleClickMs > 1200)
        titleClickCount = 0;

    lastTitleClickMs = now;
    ++titleClickCount;

    if (titleClickCount >= 5)
    {
        secretUntilMs = now + 5000;
        titleClickCount = 0;
        repaint();
    }
}
void GuitarTuneProAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto background = juce::Colour(0xff090909);
    const auto panel = juce::Colour(0xff171313);
    const auto border = juce::Colour(0xff54201c);
    const auto red = juce::Colour(0xffff3028);
    const auto orange = juce::Colour(0xffff8a30);
    const auto white = juce::Colour(0xfff4eeee);
    const auto muted = juce::Colour(0xffa18d89);

    // BLACK WOOD background
    g.setColour(background);
    g.fillAll();

    juce::ColourGradient woodBackground(
        juce::Colour(0xff241713), 0.0f, 0.0f,
        juce::Colour(0xff050505), 1000.0f, 700.0f,
        false);

    g.setGradientFill(woodBackground);
    g.fillRect(0, 0, 1000, 700);

    // Natural dark wood grain
    for (int i = 0; i < 25; ++i)
    {
        juce::Path grain;
        const float baseY = 12.0f + i * 29.0f;

        grain.startNewSubPath(0.0f, baseY);

        for (int x = 0; x <= 1000; x += 20)
        {
            const float wave =
                std::sin(x * 0.009f + i * 0.7f) * 4.0f +
                std::sin(x * 0.021f + i * 1.3f) * 1.8f;

            grain.lineTo((float)x, baseY + wave);
        }

        g.setColour(juce::Colour(0xffa16b4b).withAlpha(0.055f));
        g.strokePath(grain, juce::PathStrokeType(1.2f));
    }

    // Subtle red and orange ambient glow
    g.setColour(red.withAlpha(0.035f));
    g.fillRect(0, 0, 5, 700);

    g.setColour(orange.withAlpha(0.025f));
    g.fillRect(995, 0, 5, 700);

    // Main panels
    g.setColour(panel);
    g.fillRoundedRectangle(25.0f, 110.0f, 300.0f, 390.0f, 16.0f);
    g.fillRoundedRectangle(345.0f, 110.0f, 630.0f, 390.0f, 16.0f);
    g.fillRoundedRectangle(25.0f, 520.0f, 950.0f, 155.0f, 16.0f);

    g.setColour(border);
    g.drawRoundedRectangle(25.0f, 110.0f, 300.0f, 390.0f, 16.0f, 1.4f);
    g.drawRoundedRectangle(345.0f, 110.0f, 630.0f, 390.0f, 16.0f, 1.4f);
    g.drawRoundedRectangle(25.0f, 520.0f, 950.0f, 155.0f, 16.0f, 1.4f);

    // Red panel accents
    g.setColour(red);
    g.fillRoundedRectangle(25.0f, 110.0f, 4.0f, 390.0f, 2.0f);
    g.fillRoundedRectangle(345.0f, 110.0f, 4.0f, 390.0f, 2.0f);

    // Left tuner panel
    g.setColour(red);
    g.setFont(12.0f);
    g.drawText("LIVE PITCH", 45, 130, 250, 22,
               juce::Justification::centred);

    g.setColour(muted);
    g.setFont(10.0f);
    g.drawText("REAL-TIME FREQUENCY ANALYSIS", 40, 155, 270, 18,
               juce::Justification::centred);

    // Glowing note display
    if (signalPresent)
    {
        g.setColour(red.withAlpha(0.10f));
        g.fillEllipse(105.0f, 185.0f, 140.0f, 105.0f);
    }

    g.setColour(white.withAlpha(0.9f));
    g.setFont(juce::Font(76.0f, juce::Font::bold));
    g.drawText(noteLabel.getText(), 45, 195, 260, 90,
               juce::Justification::centred);

    const auto statusColour = tuningOk ? juce::Colour(0xff65e58d)
        : (displayCents < 0.0f ? orange : red);

    g.setColour(statusColour);
    g.setFont(juce::Font(23.0f, juce::Font::bold));
    g.drawText(centsLabel.getText(), 45, 292, 260, 38,
               juce::Justification::centred);

    g.setColour(statusColour);
    g.setFont(13.0f);
    g.drawText(hintLabel.getText(), 40, 345, 270, 28,
               juce::Justification::centred);

    g.setColour(muted);
    g.setFont(12.0f);
    g.drawText(hzLabel.getText(), 40, 390, 270, 38,
               juce::Justification::centred);

    // Fretboard heading
    g.setColour(red);
    g.setFont(12.0f);
    g.drawText("STRATOCASTER  |  SIX STRING FRETBOARD",
               370, 125, 570, 22,
               juce::Justification::centred);

    // Wooden fretboard
    const float boardX = 405.0f;
    const float boardY = 180.0f;
    const float boardW = 540.0f;
    const float boardH = 220.0f;

    juce::ColourGradient wood(
        juce::Colour(0xff633b2b), boardX, boardY,
        juce::Colour(0xff1c100d), boardX, boardY + boardH,
        false);

    g.setGradientFill(wood);
    g.fillRoundedRectangle(boardX, boardY, boardW, boardH, 8.0f);

    g.setColour(juce::Colour(0xffb77a4e).withAlpha(0.45f));
    g.drawRoundedRectangle(boardX, boardY, boardW, boardH, 8.0f, 1.2f);

    // Headstock silhouette
    juce::Path headstock;
    headstock.startNewSubPath(405.0f, 188.0f);
    headstock.lineTo(375.0f, 170.0f);
    headstock.lineTo(355.0f, 185.0f);
    headstock.lineTo(365.0f, 390.0f);
    headstock.lineTo(405.0f, 392.0f);
    headstock.closeSubPath();

    g.setColour(juce::Colour(0xff301b16));
    g.fillPath(headstock);
    g.setColour(juce::Colour(0xff8c5740));
    g.strokePath(headstock, juce::PathStrokeType(1.5f));

    // Frets
    for (int fret = 1; fret <= 22; ++fret)
    {
        const float x = boardX + boardW *
            (1.0f - std::pow(0.5f, (float)fret / 12.0f));

        g.setColour(juce::Colour(0xffd2c5b8).withAlpha(0.75f));
        g.drawLine(x, boardY + 5.0f, x, boardY + boardH - 5.0f, 2.0f);
    }

    // Nut
    g.setColour(juce::Colour(0xffe6d8c5));
    g.fillRect(boardX + 9.0f, boardY + 5.0f, 5.0f, boardH - 10.0f);

    // Fret markers
    for (int fret : {3, 5, 7, 9, 12, 15, 17, 19, 21})
    {
        const float x1 = boardX + boardW *
            (1.0f - std::pow(0.5f, (float)fret / 12.0f));
        const float x2 = boardX + boardW *
            (1.0f - std::pow(0.5f, (float)(fret + 1) / 12.0f));
        const float x = (x1 + x2) * 0.5f;

        g.setColour(juce::Colour(0xffd8c3a7).withAlpha(0.75f));

        if (fret == 12)
        {
            g.fillEllipse(x - 5.0f, 255.0f, 10.0f, 10.0f);
            g.fillEllipse(x - 5.0f, 315.0f, 10.0f, 10.0f);
        }
        else
        {
            g.fillEllipse(x - 4.0f, 285.0f, 8.0f, 8.0f);
        }
    }

    // Six strings and selected string glow
    const int selectedString = juce::jlimit(
        0, 5, stringBox.getSelectedItemIndex());

    for (int i = 0; i < 6; ++i)
    {
        const float y = 205.0f + (float)i * 35.0f;
        const float thickness = 2.6f - (float)i * 0.25f;

        if (i == selectedString)
        {
            g.setColour(red.withAlpha(0.22f));
            g.drawLine(365.0f, y, 940.0f, y, 11.0f);

            g.setColour(red);
            g.drawLine(365.0f, y, 940.0f, y, thickness + 0.8f);
        }
        else
        {
            g.setColour(juce::Colour(0xffd8d8d8).withAlpha(0.8f));
            g.drawLine(365.0f, y, 940.0f, y, thickness);
        }

        g.setColour(white);
        g.setFont(10.0f);
        g.drawText(juce::String(6 - i), 350, (int)y - 7, 16, 14,
                   juce::Justification::centred);
    }

    // Tuning status on fretboard
    g.setColour(red);
    g.setFont(11.0f);
    g.drawText("SELECTED STRING  " +
               juce::String(selectedString + 1),
               390, 410, 250, 18,
               juce::Justification::left);

    g.setColour(muted);
    g.drawText("TUNE ONE OPEN STRING AT A TIME",
               650, 410, 290, 18,
               juce::Justification::right);

    // Strobe tuner panel
    g.setColour(red);
    g.setFont(12.0f);
    g.drawText("STROBE TUNER", 370, 435, 570, 20,
               juce::Justification::centred);

    const juce::Rectangle<float> strobeArea(385.0f, 460.0f, 550.0f, 25.0f);

    g.setColour(juce::Colour(0xff080808));
    g.fillRoundedRectangle(strobeArea, 5.0f);

    g.saveState();
    g.reduceClipRegion(strobeArea.toNearestInt());

    const float offset = tuningOk ? 0.0f :
        std::fmod(animationPhase *
            (displayCents >= 0.0f ? 18.0f : -18.0f), 24.0f);

    for (float x = 360.0f + offset; x < 950.0f; x += 24.0f)
    {
        g.setColour(red.withAlpha(0.9f));
        g.fillRect(x, 460.0f, 12.0f, 25.0f);

        g.setColour(orange.withAlpha(0.8f));
        g.fillRect(x + 12.0f, 460.0f, 12.0f, 25.0f);
    }

    g.restoreState();

    g.setColour(white.withAlpha(0.9f));
    g.drawVerticalLine(660.0f, 458.0f, 487.0f);

    g.setColour(muted);
    g.setFont(9.5f);
    g.drawText("FLAT", 385, 487, 100, 12, juce::Justification::left);
    g.drawText("IN TUNE", 610, 487, 100, 12, juce::Justification::centred);
    g.drawText("SHARP", 835, 487, 100, 12, juce::Justification::right);

    // Bottom panel labels
    g.setColour(red);
    g.setFont(10.0f);
    g.drawText("CRAFTED FOR SILENTBROKEN",
               25, 680, 950, 14,
               juce::Justification::centred);

    // Hidden easter egg
    const juce::int64 now =
        (juce::int64)juce::Time::getMillisecondCounterHiRes();

    if (now < secretUntilMs)
    {
        g.setColour(juce::Colour(0xff100707));
        g.fillRoundedRectangle(juce::Rectangle<float>(325.0f, 635.0f, 350.0f, 25.0f), 7.0f);

        g.setColour(red);
        g.drawRoundedRectangle(325.0f, 635.0f, 350.0f, 25.0f, 7.0f, 1.0f);

        g.setFont(11.0f);
        g.drawText("YOU FOUND THE SILENTBROKEN FREQUENCY",
                   330, 638, 340, 18,
                   juce::Justification::centred);
    }
}

void GuitarTuneProAudioProcessorEditor::resized()
{
    title.setBounds(25, 12, 950, 38);
    subtitle.setBounds(25, 52, 950, 25);

    noteLabel.setBounds(45, 195, 260, 90);
    centsLabel.setBounds(45, 292, 260, 38);
    hintLabel.setBounds(40, 345, 270, 28);
    hzLabel.setBounds(40, 390, 270, 38);

    modeLabel.setBounds(45, 540, 170, 18);
    tuningLabel.setBounds(250, 540, 230, 18);
    stringLabel.setBounds(530, 540, 190, 18);

    modeBox.setBounds(45, 562, 170, 30);
    tuningBox.setBounds(250, 562, 230, 30);
    stringBox.setBounds(530, 562, 190, 30);

    refLabel.setBounds(45, 615, 120, 25);
    referenceSlider.setBounds(165, 612, 780, 30);
}
void GuitarTuneProAudioProcessorEditor::timerCallback()
{
    animationPhase += 0.08f;
    if (animationPhase > juce::MathConstants<float>::twoPi)
        animationPhase -= juce::MathConstants<float>::twoPi;
    const float hz = processor.detectedHz.load();
    const float confidence = processor.confidence.load();

    const int mode = (int)processor.parameters
        .getRawParameterValue("mode")->load();

    const int tuning = juce::jlimit(0, 11, (int)processor.parameters
        .getRawParameterValue("tuning")->load());

    const int stringIndex = juce::jlimit(0, 5, (int)processor.parameters
        .getRawParameterValue("string")->load());

    const float a4 = processor.parameters
        .getRawParameterValue("a4")->load();

    if (hz < 1.0f || confidence < 0.50f)
    {
        signalPresent = false;
        smoothingInitialized = false;

        noteLabel.setText("--", juce::dontSendNotification);
        centsLabel.setText("-- cents", juce::dontSendNotification);
        hintLabel.setText("PLAY ONE OPEN STRING", juce::dontSendNotification);
        hzLabel.setText("Waiting for a clear audio signal",
                        juce::dontSendNotification);

        noteLabel.setColour(juce::Label::textColourId,
                            juce::Colour(0xff8498aa));

        centsLabel.setColour(juce::Label::textColourId,
                             juce::Colour(0xff8498aa));

        repaint();
        return;
    }

    const float midiFloat = 69.0f + 12.0f * std::log2(hz / a4);
    const int nearest = (int)std::round(midiFloat);

    int target = nearest;

    if (mode == 1)
    {
        target = tunings[(size_t)tuning][(size_t)stringIndex];
    }
    else if (mode == 0)
    {
        float bestDistance = 1000.0f;

        for (int i = 0; i < 6; ++i)
        {
            const int candidate = tunings[(size_t)tuning][(size_t)i];
            const float distance = std::abs(midiFloat - (float)candidate);

            if (distance < bestDistance)
            {
                bestDistance = distance;
                target = candidate;
            }
        }
    }

    const float rawCents = 100.0f * (midiFloat - (float)target);

    if (!smoothingInitialized)
    {
        displayCents = rawCents;
        displayHz = hz;
        smoothingInitialized = true;
    }
    else
    {
        constexpr float smoothing = 0.28f;
        displayCents += smoothing * (rawCents - displayCents);
        displayHz += smoothing * (hz - displayHz);
    }

    const float cents = displayCents;

    signalPresent = true;
    tuningOk = std::abs(cents) <= 2.0f;

    const auto green = juce::Colour(0xff55e5a4);
    const auto cyan = juce::Colour(0xffff3b30);
    const auto red = juce::Colour(0xffff6b78);
    const auto colour = tuningOk ? green : (cents < 0.0f ? cyan : red);

    noteLabel.setText(
        noteName(mode == 2 ? nearest : target),
        juce::dontSendNotification);

    centsLabel.setText(
        (cents > 0.0f ? "+" : "") + juce::String(cents, 1) + " cents",
        juce::dontSendNotification);

    noteLabel.setColour(juce::Label::textColourId, colour);
    centsLabel.setColour(juce::Label::textColourId, colour);

    if (tuningOk)
        hintLabel.setText("IN TUNE  |  PERFECT",
                          juce::dontSendNotification);
    else if (cents < 0.0f)
        hintLabel.setText("TOO LOW  |  TIGHTEN THE STRING",
                          juce::dontSendNotification);
    else
        hintLabel.setText("TOO HIGH  |  LOOSEN THE STRING",
                          juce::dontSendNotification);

    hzLabel.setText(
        juce::String(displayHz, 2) + " Hz   |   Signal confidence "
        + juce::String(confidence * 100.0f, 0) + "%",
        juce::dontSendNotification);

    hintLabel.setColour(juce::Label::textColourId, colour);

    repaint();
}

