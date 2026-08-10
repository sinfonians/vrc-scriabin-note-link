// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginEditor.h"

#include <array>
#include <cmath>

namespace vrcnl
{
PluginEditor::PluginEditor(PluginProcessor& owner)
    : AudioProcessorEditor(&owner), processor(owner)
{
    setSize(620, 400);

    titleLabel.setText("VRC Scriabin Note Link", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);

    noteLabel.setText("--", juce::dontSendNotification);
    noteLabel.setFont(juce::FontOptions(72.0f, juce::Font::bold));
    noteLabel.setJustificationType(juce::Justification::centred);

    frequencyLabel.setJustificationType(juce::Justification::centred);
    confidenceLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setJustificationType(juce::Justification::centred);
    inputLabel.setText("Input", juce::dontSendNotification);
    inputLabel.setJustificationType(juce::Justification::centredRight);

    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffc857));
    noteLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    for (auto* label : { &frequencyLabel, &confidenceLabel, &statusLabel, &inputLabel })
        label->setColour(juce::Label::textColourId, juce::Colour(0xffd9dfeb));

    enableButton.setToggleState(processor.osc().enabled(), juce::dontSendNotification);
    enableButton.onClick = [this]
    {
        processor.osc().setEnabled(enableButton.getToggleState());
    };
    clearButton.onClick = [this]
    {
        processor.osc().requestAllOff();
    };

    const std::array<juce::Component*, 9> components {
        &titleLabel, &noteLabel, &frequencyLabel, &confidenceLabel, &statusLabel,
        &inputLabel, &enableButton, &clearButton, &inputMeter
    };
    for (auto* component : components)
        addAndMakeVisible(component);

    startTimerHz(20);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(0xff11141a));

    graphics.setColour(juce::Colour(0xff2a3040));
    graphics.fillRoundedRectangle(getLocalBounds().toFloat().reduced(18.0f), 12.0f);

    graphics.setColour(juce::Colour(0xffffc857));
    graphics.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    graphics.drawFittedText("Only one VRC Scriabin sender should be enabled at a time.",
                            35, getHeight() - 58, getWidth() - 70, 20,
                            juce::Justification::centred, 1);
    graphics.setColour(juce::Colour(0xffaab3c5));
    graphics.setFont(juce::FontOptions(11.0f));
    graphics.drawFittedText("Copyright 2026 asimosound | AGPL-3.0-or-later | No warranty | Source and license on GitHub",
                            35, getHeight() - 36, getWidth() - 70, 18,
                            juce::Justification::centred, 1);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced(32);
    titleLabel.setBounds(area.removeFromTop(42));
    area.removeFromTop(6);
    noteLabel.setBounds(area.removeFromTop(92));
    frequencyLabel.setBounds(area.removeFromTop(28));
    confidenceLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(10);

    auto meterRow = area.removeFromTop(28);
    inputLabel.setBounds(meterRow.removeFromLeft(70));
    inputMeter.setBounds(meterRow.reduced(6, 4));

    statusLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(8);
    auto controls = area.removeFromTop(38);
    const int controlWidth = 150;
    const int gap = 18;
    const int startX = (getWidth() - (controlWidth * 2 + gap)) / 2;
    enableButton.setBounds(startX, controls.getY(), controlWidth, controls.getHeight());
    clearButton.setBounds(startX + controlWidth + gap, controls.getY(), controlWidth, controls.getHeight());
}

void PluginEditor::timerCallback()
{
    auto& osc = processor.osc();
    enableButton.setToggleState(osc.enabled(), juce::dontSendNotification);

    const int pitchClass = osc.activePitchClass();
    noteLabel.setText(noteText(pitchClass), juce::dontSendNotification);

    const float frequency = osc.displayedFrequencyHz();
    frequencyLabel.setText(frequency > 0.0f ? juce::String(frequency, 1) + " Hz" : "-- Hz",
                           juce::dontSendNotification);
    confidenceLabel.setText("Confidence " + juce::String(osc.displayedConfidence() * 100.0f, 0) + "%",
                            juce::dontSendNotification);
    statusLabel.setText(statusText(osc.status()), juce::dontSendNotification);

    const float rms = processor.inputRms();
    meterValue = juce::jlimit(0.0, 1.0, static_cast<double>(rms * 5.0f));
}

juce::String PluginEditor::statusText(VrchatOscSender::Status status)
{
    switch (status)
    {
        case VrchatOscSender::Status::disabled: return "OSC is off (127.0.0.1:9000)";
        case VrchatOscSender::Status::waitingForAudio: return "OSC on - waiting for a clear monophonic input";
        case VrchatOscSender::Status::ownershipUnavailable: return "OSC blocked - another Note Link instance owns the sender";
        case VrchatOscSender::Status::connectionFailed: return "OSC connection failed";
        case VrchatOscSender::Status::sending: return "Sending one pitch class to VRChat";
    }
    return "Unknown status";
}

juce::String PluginEditor::noteText(int pitchClass)
{
    static constexpr std::array<const char*, 12> names {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    if (pitchClass < 0 || pitchClass >= static_cast<int>(names.size()))
        return "--";
    return names[static_cast<std::size_t>(pitchClass)];
}
} // namespace vrcnl
