// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <atomic>
#include <cstdint>

namespace vrcnl
{
struct DetectionSnapshot
{
    float frequencyHz = 0.0f;
    float confidence = 0.0f;
    float rms = 0.0f;
    bool voiced = false;
    std::uint32_t generation = 0;
};

class DetectionMailbox
{
public:
    static_assert(std::atomic<float>::is_always_lock_free,
                  "Real-time publication requires lock-free float atomics");
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
                  "Real-time publication requires lock-free integer atomics");
    static_assert(std::atomic<bool>::is_always_lock_free,
                  "Real-time publication requires lock-free boolean atomics");

    void publish(const DetectionSnapshot& value) noexcept
    {
        const auto start = sequence.load(std::memory_order_relaxed);
        sequence.store(start + 1u, std::memory_order_relaxed);
        frequencyHz.store(value.frequencyHz, std::memory_order_relaxed);
        confidence.store(value.confidence, std::memory_order_relaxed);
        rms.store(value.rms, std::memory_order_relaxed);
        voiced.store(value.voiced, std::memory_order_relaxed);
        generation.store(value.generation, std::memory_order_relaxed);
        sequence.store(start + 2u, std::memory_order_release);
    }

    [[nodiscard]] bool tryRead(DetectionSnapshot& result) const noexcept
    {
        for (int attempt = 0; attempt < 32; ++attempt)
        {
            const auto before = sequence.load(std::memory_order_acquire);
            if ((before & 1u) != 0u)
                continue;

            DetectionSnapshot candidate;
            candidate.frequencyHz = frequencyHz.load(std::memory_order_relaxed);
            candidate.confidence = confidence.load(std::memory_order_relaxed);
            candidate.rms = rms.load(std::memory_order_relaxed);
            candidate.voiced = voiced.load(std::memory_order_relaxed);
            candidate.generation = generation.load(std::memory_order_relaxed);

            const auto after = sequence.load(std::memory_order_acquire);
            if (before == after && (after & 1u) == 0u)
            {
                result = candidate;
                return true;
            }
        }
        return false;
    }

private:
    alignas(64) std::atomic<std::uint32_t> sequence { 0 };
    std::atomic<float> frequencyHz { 0.0f };
    std::atomic<float> confidence { 0.0f };
    std::atomic<float> rms { 0.0f };
    std::atomic<bool> voiced { false };
    std::atomic<std::uint32_t> generation { 0 };
};
} // namespace vrcnl
