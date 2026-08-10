// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Core/DetectionMailbox.h"
#include "Core/FundamentalDetector.h"
#include "Core/NoteStabilizer.h"

#include <atomic>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <thread>
#include <vector>

namespace
{
std::atomic<bool> trackAllocations { false };
std::atomic<std::size_t> allocationCount { 0 };
}

void* operator new(std::size_t size)
{
    if (trackAllocations.load(std::memory_order_relaxed))
        allocationCount.fetch_add(1, std::memory_order_relaxed);
    if (void* memory = std::malloc(size))
        return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

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

void testFundamentalAccuracy()
{
    for (const double sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        for (const int blockSize : { 64, 127, 512 })
        {
            for (const float expected : { 82.4069f, 110.0f, 220.0f, 440.0f, 880.0f })
            {
                vrcnl::FundamentalDetector detector;
                detector.prepare(sampleRate);
                std::vector<float> block(static_cast<std::size_t>(blockSize));
                double phase = 0.0;
                const double increment = 2.0 * 3.14159265358979323846 * expected / sampleRate;
                const int blockCount = static_cast<int>(sampleRate * 1.2 / blockSize);
                for (int blockIndex = 0; blockIndex < blockCount; ++blockIndex)
                {
                    for (float& sample : block)
                    {
                        sample = 0.25f * static_cast<float>(std::sin(phase));
                        phase += increment;
                        if (phase >= 2.0 * 3.14159265358979323846)
                            phase -= 2.0 * 3.14159265358979323846;
                    }
                    detector.process(block.data(), blockSize);
                }

                const float errorRatio = std::abs(detector.frequencyHz() - expected) / expected;
                require(detector.voiced(), "sine should be voiced");
                require(detector.confidence() >= 0.80f, "sine confidence should be high");
                require(errorRatio < 0.018f,
                        "frequency error exceeds 1.8% at " + std::to_string(expected)
                        + " Hz, sr=" + std::to_string(sampleRate)
                        + ", block=" + std::to_string(blockSize)
                        + ", got=" + std::to_string(detector.frequencyHz()));
            }
        }
    }
}

void testHarmonicRichFundamental()
{
    for (const float expected : { 82.4069f, 110.0f, 220.0f, 440.0f })
    {
        vrcnl::FundamentalDetector detector;
        detector.prepare(48000.0);
        std::vector<float> block(128);
        double phase = 0.0;
        const double increment = 2.0 * 3.14159265358979323846 * expected / 48000.0;
        for (int blockIndex = 0; blockIndex < 450; ++blockIndex)
        {
            for (float& sample : block)
            {
                sample = 0.04f * static_cast<float>(std::sin(phase))
                       + 0.22f * static_cast<float>(std::sin(2.0 * phase))
                       + 0.10f * static_cast<float>(std::sin(3.0 * phase));
                phase += increment;
                if (phase >= 2.0 * 3.14159265358979323846)
                    phase -= 2.0 * 3.14159265358979323846;
            }
            detector.process(block.data(), static_cast<int>(block.size()));
        }
        const float errorRatio = std::abs(detector.frequencyHz() - expected) / expected;
        require(detector.voiced(), "harmonic-rich tone should be voiced");
        require(errorRatio < 0.025f,
                "harmonic-rich tone fundamental mismatch at " + std::to_string(expected)
                + " Hz, got=" + std::to_string(detector.frequencyHz()));
    }
}

void testSilenceAndAllocationContract()
{
    vrcnl::FundamentalDetector detector;
    detector.prepare(48000.0);
    std::vector<float> silence(512, 0.0f);
    for (int i = 0; i < 120; ++i)
        detector.process(silence.data(), static_cast<int>(silence.size()));
    require(!detector.voiced(), "silence must remain unvoiced");
    require(detector.frequencyHz() == 0.0f, "silence frequency must be zero");

    std::uint32_t noiseState = 0x12345678u;
    std::vector<float> noise(512);
    for (int block = 0; block < 180; ++block)
    {
        for (float& sample : noise)
        {
            noiseState = noiseState * 1664525u + 1013904223u;
            sample = (static_cast<float>((noiseState >> 8) & 0xffffu) / 32767.5f - 1.0f) * 0.05f;
        }
        detector.process(noise.data(), static_cast<int>(noise.size()));
    }
    require(!detector.voiced(), "deterministic broadband noise must not remain voiced");

    std::vector<float> tone(512);
    for (std::size_t i = 0; i < tone.size(); ++i)
        tone[i] = 0.2f * std::sin(static_cast<float>(i) * 2.0f * 3.14159265f * 220.0f / 48000.0f);
    detector.process(tone.data(), static_cast<int>(tone.size()));

    allocationCount.store(0, std::memory_order_relaxed);
    trackAllocations.store(true, std::memory_order_release);
    for (int i = 0; i < 100; ++i)
        detector.process(tone.data(), static_cast<int>(tone.size()));
    trackAllocations.store(false, std::memory_order_release);
    require(allocationCount.load(std::memory_order_relaxed) == 0,
            "prepared detector processing must allocate zero heap blocks");
}

void testNoteStabilizer()
{
    vrcnl::NoteStabilizer stabilizer;
    auto result = stabilizer.update({ 440.0f, 0.95f, 0.1f, true, 1 }, 0.0);
    require(result.pitchClass == 9, "A4 must map to PC9");

    result = stabilizer.update({ 261.6256f, 0.95f, 0.1f, true, 2 }, 50.0);
    require(result.pitchClass == 9, "note transition must require confirmation");
    result = stabilizer.update({ 261.6256f, 0.95f, 0.1f, true, 3 }, 100.0);
    require(result.pitchClass == 0, "confirmed middle C must map to PC0");

    result = stabilizer.update({ 0.0f, 0.0f, 0.0f, false, 4 }, 150.0);
    require(result.pitchClass == 0, "short dropout must honor hold window");
    result = stabilizer.update({ 0.0f, 0.0f, 0.0f, false, 5 }, 201.0);
    require(!result.pitchClass.has_value(), "sustained silence must clear the note");

    result = stabilizer.update({ 440.0f, 0.49f, 0.1f, true, 6 }, 300.0);
    require(!result.valid, "low confidence must be rejected");
    result = stabilizer.update({ 440.0f, 0.99f, 0.003f, true, 7 }, 400.0);
    require(!result.valid, "sub-threshold RMS must be rejected");
}

void testMailboxConsistency()
{
    vrcnl::DetectionMailbox mailbox;
    std::atomic<bool> done { false };
    std::thread writer([&]
    {
        for (std::uint32_t generation = 1; generation <= 200000; ++generation)
        {
            mailbox.publish({ static_cast<float>(generation),
                              static_cast<float>(generation * 2u),
                              static_cast<float>(generation * 3u),
                              (generation & 1u) != 0u,
                              generation });
            if ((generation & 63u) == 0u)
                std::this_thread::yield();
        }
        done.store(true, std::memory_order_release);
    });

    std::size_t successfulReads = 0;
    while (!done.load(std::memory_order_acquire))
    {
        vrcnl::DetectionSnapshot snapshot;
        if (!mailbox.tryRead(snapshot) || snapshot.generation == 0)
            continue;
        const auto generation = snapshot.generation;
        require(snapshot.frequencyHz == static_cast<float>(generation), "mailbox frequency generation mismatch");
        require(snapshot.confidence == static_cast<float>(generation * 2u), "mailbox confidence generation mismatch");
        require(snapshot.rms == static_cast<float>(generation * 3u), "mailbox RMS generation mismatch");
        require(snapshot.voiced == ((generation & 1u) != 0u), "mailbox voiced generation mismatch");
        ++successfulReads;
        if (failures != 0)
            break;
    }
    writer.join();
    require(successfulReads >= 100, "mailbox stress must complete at least 100 consistent reads");

    vrcnl::DetectionSnapshot finalSnapshot;
    require(mailbox.tryRead(finalSnapshot), "mailbox final snapshot must be readable");
    require(finalSnapshot.generation == 200000u, "mailbox final generation must match the writer");
}
} // namespace

int main()
{
    testFundamentalAccuracy();
    testHarmonicRichFundamental();
    testSilenceAndAllocationContract();
    testNoteStabilizer();
    testMailboxConsistency();

    if (failures == 0)
    {
        std::cout << "All core contracts passed.\n";
        return 0;
    }
    std::cerr << failures << " core contract(s) failed.\n";
    return 1;
}
