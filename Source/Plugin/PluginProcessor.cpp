// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace vrcnl
{
namespace
{
constexpr std::uint32_t stateMagic = 0x56534E4Cu; // VSNL
constexpr std::uint32_t stateSchemaVersion = 1u;
}

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      oscSender(detectionMailbox)
{
    setLatencySamples(0);
}

void PluginProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    preparedMaximumBlockSize = std::max(1, maximumExpectedSamplesPerBlock);
    monoScratch.assign(static_cast<std::size_t>(preparedMaximumBlockSize), 0.0f);
    detector.prepare(sampleRate);
    generation = 0;
    publicInputRms.store(0.0f, std::memory_order_relaxed);
}

void PluginProcessor::releaseResources()
{
    publicInputRms.store(0.0f, std::memory_order_relaxed);
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    if (input != output)
        return false;
    return input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo();
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int sampleCount = buffer.getNumSamples();
    const int inputChannels = getTotalNumInputChannels();

    if (sampleCount <= 0 || inputChannels <= 0 || sampleCount > preparedMaximumBlockSize
        || static_cast<int>(monoScratch.size()) < sampleCount)
    {
        publishInvalid();
        if (wrapperType == wrapperType_Standalone)
            buffer.clear();
        return;
    }

    double energy = 0.0;
    const float scale = 1.0f / static_cast<float>(inputChannels);
    for (int sample = 0; sample < sampleCount; ++sample)
    {
        float mono = 0.0f;
        for (int channel = 0; channel < inputChannels; ++channel)
            mono += buffer.getReadPointer(channel)[sample];
        mono *= scale;
        monoScratch[static_cast<std::size_t>(sample)] = mono;
        energy += static_cast<double>(mono) * mono;
    }

    const float rms = static_cast<float>(std::sqrt(energy / sampleCount));
    detector.process(monoScratch.data(), sampleCount);
    detectionMailbox.publish({ detector.frequencyHz(), detector.confidence(), rms,
                               detector.voiced(), ++generation });
    publicInputRms.store(rms, std::memory_order_relaxed);

    for (int channel = inputChannels; channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, sampleCount);

    if (wrapperType == wrapperType_Standalone)
        buffer.clear();
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destinationData)
{
    juce::MemoryOutputStream stream(destinationData, false);
    stream.writeInt(static_cast<int>(stateMagic));
    stream.writeInt(static_cast<int>(stateSchemaVersion));
    // Network sending is deliberately never persisted.
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // A restore attempt always fails safe, even if the blob is corrupt or from
    // an unsupported future schema.
    oscSender.setEnabled(false);

    if (data == nullptr || sizeInBytes < 8)
        return;

    juce::MemoryInputStream stream(data, static_cast<std::size_t>(sizeInBytes), false);
    const auto magic = static_cast<std::uint32_t>(stream.readInt());
    const auto version = static_cast<std::uint32_t>(stream.readInt());
    if (magic != stateMagic || version > stateSchemaVersion)
        return;

}

void PluginProcessor::publishInvalid() noexcept
{
    detectionMailbox.publish({ 0.0f, 0.0f, 0.0f, false, ++generation });
    publicInputRms.store(0.0f, std::memory_order_relaxed);
}
} // namespace vrcnl

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new vrcnl::PluginProcessor();
}
