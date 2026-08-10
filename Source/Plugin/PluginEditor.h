// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include <JuceHeader.h>

namespace vrcnl
{
class PluginEditor final : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    static juce::String statusText(VrchatOscSender::Status status);
    static juce::String noteText(int pitchClass);

    PluginProcessor& processor;
    juce::Label titleLabel;
    juce::Label noteLabel;
    juce::Label frequencyLabel;
    juce::Label confidenceLabel;
    juce::Label statusLabel;
    juce::Label inputLabel;
    juce::ToggleButton enableButton { "Enable OSC" };
    juce::TextButton clearButton { "All Off" };
    double meterValue = 0.0;
    juce::ProgressBar inputMeter { meterValue };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
} // namespace vrcnl
