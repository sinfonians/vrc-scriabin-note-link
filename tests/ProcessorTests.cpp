// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Plugin/PluginProcessor.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <vector>

namespace
{
thread_local bool trackAllocations = false;
thread_local std::size_t allocationCount = 0;
}

void* operator new(std::size_t size)
{
    if (trackAllocations)
        ++allocationCount;
    if (void* memory = std::malloc(size))
        return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

namespace
{
int failures = 0;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void fillInput(juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            buffer.setSample(channel, sample,
                             0.2f * std::sin(static_cast<float>(sample) * 0.031f + channel * 0.3f));
}

void testVstTransparencyAndRealtimeContract()
{
    juce::AudioProcessor::setTypeOfNextNewPlugin(juce::AudioProcessor::wrapperType_VST3);
    vrcnl::PluginProcessor processor;
    require(processor.wrapperType == juce::AudioProcessor::wrapperType_VST3,
            "processor must be constructed as VST3 for transparency test");
    require(processor.getLatencySamples() == 0, "VST3 must report zero latency");
    processor.prepareToPlay(48000.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    std::vector<float> expected(static_cast<std::size_t>(buffer.getNumChannels() * buffer.getNumSamples()));

    allocationCount = 0;
    for (int iteration = 0; iteration < 100; ++iteration)
    {
        fillInput(buffer);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                expected[static_cast<std::size_t>(channel * buffer.getNumSamples() + sample)]
                    = buffer.getSample(channel, sample);

        trackAllocations = true;
        processor.processBlock(buffer, midi);
        trackAllocations = false;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                require(buffer.getSample(channel, sample)
                            == expected[static_cast<std::size_t>(channel * buffer.getNumSamples() + sample)],
                        "VST3 output must be sample-transparent");
    }
    require(allocationCount == 0,
            "prepared PluginProcessor::processBlock must allocate zero heap blocks; got "
                + std::to_string(allocationCount));

    juce::AudioBuffer<float> oversized(2, 1024);
    fillInput(oversized);
    allocationCount = 0;
    trackAllocations = true;
    processor.processBlock(oversized, midi);
    trackAllocations = false;
    require(allocationCount == 0,
            "oversized fail-safe must not allocate; got " + std::to_string(allocationCount));
}

void testStandaloneSilence()
{
    juce::AudioProcessor::setTypeOfNextNewPlugin(juce::AudioProcessor::wrapperType_Standalone);
    vrcnl::PluginProcessor processor;
    require(processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone,
            "processor must be constructed as Standalone for silence test");
    processor.prepareToPlay(48000.0, 256);

    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    fillInput(buffer);
    processor.processBlock(buffer, midi);
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            require(buffer.getSample(channel, sample) == 0.0f,
                    "Standalone output must always be silent");

    juce::AudioBuffer<float> oversized(2, 512);
    fillInput(oversized);
    processor.processBlock(oversized, midi);
    for (int channel = 0; channel < oversized.getNumChannels(); ++channel)
        for (int sample = 0; sample < oversized.getNumSamples(); ++sample)
            require(oversized.getSample(channel, sample) == 0.0f,
                    "Standalone oversized fail-safe must still be silent");
}

void testStateAlwaysDisablesOsc()
{
    juce::AudioProcessor::setTypeOfNextNewPlugin(juce::AudioProcessor::wrapperType_VST3);
    vrcnl::PluginProcessor processor;
    require(!processor.osc().enabled(), "OSC must default off after construction");

    juce::MemoryBlock validState;
    processor.getStateInformation(validState);
    require(validState.getSize() == 8, "schema v1 state must contain only magic and version");

    processor.osc().setEnabled(true);
    processor.setStateInformation(validState.getData(), static_cast<int>(validState.getSize()));
    require(!processor.osc().enabled(), "valid state restore must disable OSC");

    const std::uint32_t corrupt = 0x12345678u;
    processor.osc().setEnabled(true);
    processor.setStateInformation(&corrupt, static_cast<int>(sizeof(corrupt)));
    require(!processor.osc().enabled(), "short/corrupt state restore must disable OSC");

    std::array<std::uint32_t, 2> futureState { 0x56534E4Cu, 999u };
    processor.osc().setEnabled(true);
    processor.setStateInformation(futureState.data(), static_cast<int>(sizeof(futureState)));
    require(!processor.osc().enabled(), "unsupported future state restore must disable OSC");
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    testVstTransparencyAndRealtimeContract();
    testStandaloneSilence();
    testStateAlwaysDisablesOsc();

    if (failures == 0)
    {
        std::cout << "All processor contracts passed.\n";
        return 0;
    }
    std::cerr << failures << " processor contract(s) failed.\n";
    return 1;
}
