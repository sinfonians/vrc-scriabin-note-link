// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "Core/DetectionMailbox.h"
#include "Core/FundamentalDetector.h"
#include "Osc/VrchatOscSender.h"

#include <JuceHeader.h>

#include <atomic>
#include <vector>

namespace vrcnl
{
class PluginProcessor final : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override = default;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destinationData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    VrchatOscSender& osc() noexcept { return oscSender; }
    [[nodiscard]] float inputRms() const noexcept { return publicInputRms.load(std::memory_order_relaxed); }

private:
    void publishInvalid() noexcept;

    DetectionMailbox detectionMailbox;
    FundamentalDetector detector;
    VrchatOscSender oscSender;
    std::vector<float> monoScratch;
    std::atomic<float> publicInputRms { 0.0f };
    std::uint32_t generation = 0;
    int preparedMaximumBlockSize = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
} // namespace vrcnl
