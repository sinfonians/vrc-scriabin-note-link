// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "DetectionMailbox.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

namespace vrcnl
{
class NoteStabilizer
{
public:
    struct Result
    {
        std::optional<int> midiNote;
        std::optional<int> pitchClass;
        float frequencyHz = 0.0f;
        float confidence = 0.0f;
        bool valid = false;
    };

    Result update(const DetectionSnapshot& detection, double nowMs) noexcept
    {
        const bool validInput = detection.voiced
            && std::isfinite(detection.frequencyHz)
            && detection.frequencyHz >= minFrequencyHz
            && detection.frequencyHz <= maxFrequencyHz
            && std::isfinite(detection.confidence)
            && detection.confidence >= minimumConfidence
            && std::isfinite(detection.rms)
            && detection.rms >= minimumRms;

        if (!validInput)
        {
            pendingNote.reset();
            pendingCount = 0;
            if (stableNote.has_value() && nowMs < holdUntilMs)
                return makeResult(detection, true);
            clear();
            return makeResult(detection, false);
        }

        const float midiFloat = 69.0f + 12.0f * std::log2(detection.frequencyHz / 440.0f);
        int candidate = static_cast<int>(std::lround(midiFloat));

        if (stableNote.has_value())
        {
            const float distanceFromStable = std::abs(midiFloat - static_cast<float>(*stableNote));
            if (distanceFromStable < semitoneExitThreshold)
                candidate = *stableNote;
        }

        if (!stableNote.has_value())
        {
            stableNote = candidate;
            pendingNote.reset();
            pendingCount = 0;
        }
        else if (candidate != *stableNote)
        {
            if (pendingNote == candidate)
                ++pendingCount;
            else
            {
                pendingNote = candidate;
                pendingCount = 1;
            }

            if (pendingCount >= transitionConfirmations)
            {
                stableNote = candidate;
                pendingNote.reset();
                pendingCount = 0;
            }
        }
        else
        {
            pendingNote.reset();
            pendingCount = 0;
        }

        lastValidFrequency = detection.frequencyHz;
        lastValidConfidence = detection.confidence;
        holdUntilMs = nowMs + holdMilliseconds;
        return makeResult(detection, true);
    }

    void clear() noexcept
    {
        stableNote.reset();
        pendingNote.reset();
        pendingCount = 0;
        holdUntilMs = 0.0;
        lastValidFrequency = 0.0f;
        lastValidConfidence = 0.0f;
    }

    static constexpr float minFrequencyHz = 70.0f;
    static constexpr float maxFrequencyHz = 1000.0f;
    static constexpr float minimumConfidence = 0.50f;
    static constexpr float minimumRms = 0.004f; // approximately -48 dBFS
    static constexpr float semitoneExitThreshold = 0.60f;
    static constexpr int transitionConfirmations = 2;
    static constexpr double holdMilliseconds = 100.0;

private:
    Result makeResult(const DetectionSnapshot& detection, bool valid) const noexcept
    {
        Result result;
        result.valid = valid && stableNote.has_value();
        result.midiNote = result.valid ? stableNote : std::nullopt;
        if (result.midiNote.has_value())
        {
            int pitchClass = *result.midiNote % 12;
            if (pitchClass < 0)
                pitchClass += 12;
            result.pitchClass = pitchClass;
        }
        result.frequencyHz = detection.voiced ? detection.frequencyHz : lastValidFrequency;
        result.confidence = detection.voiced ? detection.confidence : lastValidConfidence;
        return result;
    }

    std::optional<int> stableNote;
    std::optional<int> pendingNote;
    int pendingCount = 0;
    double holdUntilMs = 0.0;
    float lastValidFrequency = 0.0f;
    float lastValidConfidence = 0.0f;
};
} // namespace vrcnl
