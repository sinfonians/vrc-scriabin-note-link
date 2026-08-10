// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "Core/DetectionMailbox.h"
#include "Core/NoteStabilizer.h"

#include <juce_events/juce_events.h>
#include <juce_osc/juce_osc.h>

#include <array>
#include <atomic>

namespace vrcnl
{
class VrchatOscSender final : private juce::Timer
{
public:
    enum class Status : int
    {
        disabled,
        waitingForAudio,
        ownershipUnavailable,
        connectionFailed,
        sending
    };

    explicit VrchatOscSender(const DetectionMailbox& source);
#if defined(VRCNL_TESTING)
    VrchatOscSender(const DetectionMailbox& source, juce::String testHost, int testPort);
#endif
    ~VrchatOscSender() override;

    void setEnabled(bool shouldEnable) noexcept;
    void requestAllOff() noexcept;

    [[nodiscard]] bool enabled() const noexcept { return wantEnabled.load(std::memory_order_relaxed); }
    [[nodiscard]] bool ownsSender() const noexcept { return owns.load(std::memory_order_relaxed); }
    [[nodiscard]] bool connectedToTarget() const noexcept { return connectedState.load(std::memory_order_relaxed); }
    [[nodiscard]] Status status() const noexcept { return static_cast<Status>(publicStatus.load(std::memory_order_relaxed)); }
    [[nodiscard]] int activePitchClass() const noexcept { return publicPitchClass.load(std::memory_order_relaxed); }
    [[nodiscard]] float displayedFrequencyHz() const noexcept { return publicFrequency.load(std::memory_order_relaxed); }
    [[nodiscard]] float displayedConfidence() const noexcept { return publicConfidence.load(std::memory_order_relaxed); }

    static constexpr int destinationPort = 9000;
    static constexpr int timerFrequencyHz = 20;
    static constexpr double heartbeatMilliseconds = 1000.0;
    static constexpr double staleDetectionMilliseconds = 150.0;

private:
    void timerCallback() override;
    bool acquireAndConnect();
    void disableAndRelease();
    void performOneShotClear();
    void sendMask(std::uint16_t mask, bool force);
    void updatePublicDetection(const NoteStabilizer::Result& result) noexcept;

    const DetectionMailbox& mailbox;
    NoteStabilizer stabilizer;
    juce::OSCSender sender;
    juce::String host { "127.0.0.1" };
    int port = destinationPort;

    std::atomic<bool> wantEnabled { false };
    std::atomic<bool> clearRequested { false };
    std::atomic<bool> owns { false };
    std::atomic<bool> connectedState { false };
    std::atomic<int> publicStatus { static_cast<int>(Status::disabled) };
    std::atomic<int> publicPitchClass { -1 };
    std::atomic<float> publicFrequency { 0.0f };
    std::atomic<float> publicConfidence { 0.0f };

    bool socketConnected = false;
    std::uint16_t lastSentMask = 0;
    double lastHeartbeatMs = 0.0;
    DetectionSnapshot lastCompleteSnapshot;
    std::uint32_t lastGeneration = 0;
    double lastGenerationTimeMs = 0.0;

    static constexpr std::array<const char*, 12> addresses {
        "/avatar/parameters/PC0",  "/avatar/parameters/PC1",
        "/avatar/parameters/PC2",  "/avatar/parameters/PC3",
        "/avatar/parameters/PC4",  "/avatar/parameters/PC5",
        "/avatar/parameters/PC6",  "/avatar/parameters/PC7",
        "/avatar/parameters/PC8",  "/avatar/parameters/PC9",
        "/avatar/parameters/PC10", "/avatar/parameters/PC11"
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VrchatOscSender)
};
} // namespace vrcnl
