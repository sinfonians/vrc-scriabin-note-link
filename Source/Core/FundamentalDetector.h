// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace vrcnl
{
class FundamentalDetector
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = std::max(8000.0, newSampleRate);
        minLag = std::max(2, static_cast<int>(sampleRate / maxFrequencyHz));
        maxLag = std::max(minLag + 1, static_cast<int>(sampleRate / minFrequencyHz));

        int size = 1;
        while (size < 3 * maxLag + 2048)
            size <<= 1;

        history.assign(static_cast<std::size_t>(size), 0.0f);
        mask = size - 1;
        detectionHop = std::max(64, static_cast<int>(sampleRate / 250.0));
        reset();
    }

    void reset() noexcept
    {
        std::fill(history.begin(), history.end(), 0.0f);
        writePosition = 0;
        samplesUntilDetection = detectionHop;
        smoothedPeriod = static_cast<float>(std::clamp(static_cast<int>(sampleRate / 180.0), minLag, maxLag));
        currentConfidence = 0.0f;
        currentlyVoiced = false;
    }

    void process(const float* samples, int count) noexcept
    {
        if (samples == nullptr || count <= 0 || history.empty())
            return;

        for (int i = 0; i < count; ++i)
        {
            history[static_cast<std::size_t>(writePosition & mask)] = samples[i];
            ++writePosition;
            if (--samplesUntilDetection <= 0)
            {
                samplesUntilDetection = detectionHop;
                detect();
            }
        }
    }

    [[nodiscard]] float frequencyHz() const noexcept
    {
        return currentlyVoiced && smoothedPeriod > 1.0f
            ? static_cast<float>(sampleRate / smoothedPeriod)
            : 0.0f;
    }

    [[nodiscard]] float confidence() const noexcept { return currentConfidence; }
    [[nodiscard]] bool voiced() const noexcept { return currentlyVoiced; }

    static constexpr float minFrequencyHz = 70.0f;
    static constexpr float maxFrequencyHz = 1000.0f;

private:
    void detect() noexcept
    {
        const int window = 2 * maxLag;
        if (writePosition < static_cast<long long>(window + maxLag))
            return;

        const long long base = writePosition - window;
        constexpr int sampleStride = 2;
        double referenceEnergy = 0.0;
        for (int i = 0; i < window; i += sampleStride)
        {
            const float value = history[static_cast<std::size_t>((base + i) & mask)];
            referenceEnergy += static_cast<double>(value) * value;
        }

        if (referenceEnergy < 1.0e-6)
        {
            currentlyVoiced = false;
            currentConfidence = 0.0f;
            return;
        }

        const auto correlationAt = [this, base, window, referenceEnergy](int lag) noexcept
        {
            double cross = 0.0;
            double lagEnergy = 0.0;
            for (int i = 0; i < window; i += sampleStride)
            {
                const float a = history[static_cast<std::size_t>((base + i) & mask)];
                const float b = history[static_cast<std::size_t>((base + i - lag) & mask)];
                cross += static_cast<double>(a) * b;
                lagEnergy += static_cast<double>(b) * b;
            }
            return static_cast<float>(cross / std::sqrt(referenceEnergy * lagEnergy + 1.0e-12));
        };

        constexpr int coarseStep = 4;
        float bestCorrelation = 0.0f;
        int bestLag = minLag;
        for (int lag = minLag; lag <= maxLag; lag += coarseStep)
        {
            const float correlation = correlationAt(lag);
            if (correlation > bestCorrelation)
            {
                bestCorrelation = correlation;
                bestLag = lag;
            }
        }

        // A periodic signal also has strong peaks at integer multiples of its true
        // period. Prefer the shortest candidate that is nearly as strong as the
        // global peak so a high note is not mistaken for an octave/subharmonic.
        for (int lag = minLag; lag <= maxLag; lag += coarseStep)
        {
            if (correlationAt(lag) >= 0.92f * bestCorrelation)
            {
                bestLag = lag;
                break;
            }
        }

        float refinedCorrelation = 0.0f;
        int refinedLag = bestLag;
        for (int lag = std::max(minLag, bestLag - coarseStep);
             lag <= std::min(maxLag, bestLag + coarseStep); ++lag)
        {
            const float correlation = correlationAt(lag);
            if (correlation > refinedCorrelation)
            {
                refinedCorrelation = correlation;
                refinedLag = lag;
            }
        }

        // A strong upper harmonic can make half of the true period look valid.
        // Prefer a longer candidate only when it is measurably stronger; an
        // equal-strength integer multiple of a pure sine must not win.
        const int shortestStrongLag = refinedLag;
        for (int multiplier = 2; multiplier <= 4; ++multiplier)
        {
            const int centre = shortestStrongLag * multiplier;
            if (centre > maxLag)
                break;

            float longerCorrelation = 0.0f;
            int longerLag = centre;
            for (int lag = std::max(minLag, centre - coarseStep);
                 lag <= std::min(maxLag, centre + coarseStep); ++lag)
            {
                const float correlation = correlationAt(lag);
                if (correlation > longerCorrelation)
                {
                    longerCorrelation = correlation;
                    longerLag = lag;
                }
            }
            if (longerCorrelation > refinedCorrelation + 0.015f)
            {
                refinedCorrelation = longerCorrelation;
                refinedLag = longerLag;
            }
        }

        if (currentlyVoiced)
        {
            const int previousLag = static_cast<int>(std::lround(smoothedPeriod));
            float nearbyCorrelation = 0.0f;
            int nearbyLag = previousLag;
            for (int lag = std::max(minLag, previousLag - coarseStep);
                 lag <= std::min(maxLag, previousLag + coarseStep); ++lag)
            {
                const float correlation = correlationAt(lag);
                if (correlation > nearbyCorrelation)
                {
                    nearbyCorrelation = correlation;
                    nearbyLag = lag;
                }
            }
            if (nearbyCorrelation >= 0.85f * refinedCorrelation)
            {
                refinedCorrelation = nearbyCorrelation;
                refinedLag = nearbyLag;
            }
        }

        currentConfidence = std::clamp(refinedCorrelation, 0.0f, 1.0f);
        const bool wasVoiced = currentlyVoiced;
        const bool voicedNow = wasVoiced ? refinedCorrelation > 0.50f
                                         : refinedCorrelation > 0.62f;
        if (voicedNow && refinedLag > 0)
        {
            currentlyVoiced = true;
            smoothedPeriod = wasVoiced
                ? 0.6f * smoothedPeriod + 0.4f * static_cast<float>(refinedLag)
                : static_cast<float>(refinedLag);
        }
        else
        {
            currentlyVoiced = false;
        }
    }

    double sampleRate = 48000.0;
    int minLag = 48;
    int maxLag = 686;
    int mask = 0;
    int detectionHop = 192;
    int samplesUntilDetection = 192;
    long long writePosition = 0;
    std::vector<float> history;
    float smoothedPeriod = 220.0f;
    float currentConfidence = 0.0f;
    bool currentlyVoiced = false;
};
} // namespace vrcnl
