// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "VrchatOscSender.h"

#include "SenderOwnership.h"

#include <cmath>

namespace vrcnl
{
VrchatOscSender::VrchatOscSender(const DetectionMailbox& source)
    : mailbox(source)
{
    startTimerHz(timerFrequencyHz);
}

#if defined(VRCNL_TESTING)
VrchatOscSender::VrchatOscSender(const DetectionMailbox& source, juce::String testHost, int testPort)
    : mailbox(source), host(std::move(testHost)), port(testPort)
{
    startTimerHz(timerFrequencyHz);
}
#endif

VrchatOscSender::~VrchatOscSender()
{
    stopTimer();
    disableAndRelease();
}

void VrchatOscSender::setEnabled(bool shouldEnable) noexcept
{
    wantEnabled.store(shouldEnable, std::memory_order_relaxed);
}

void VrchatOscSender::requestAllOff() noexcept
{
    wantEnabled.store(false, std::memory_order_relaxed);
    clearRequested.store(true, std::memory_order_release);
}

void VrchatOscSender::timerCallback()
{
    if (clearRequested.exchange(false, std::memory_order_acq_rel))
        performOneShotClear();

    if (!wantEnabled.load(std::memory_order_relaxed))
    {
        disableAndRelease();
        publicStatus.store(static_cast<int>(Status::disabled), std::memory_order_relaxed);
        return;
    }

    if (!acquireAndConnect())
        return;

    const double now = juce::Time::getMillisecondCounterHiRes();
    DetectionSnapshot candidate;
    if (mailbox.tryRead(candidate))
    {
        lastCompleteSnapshot = candidate;
        if (candidate.generation != lastGeneration)
        {
            lastGeneration = candidate.generation;
            lastGenerationTimeMs = now;
        }
    }

    DetectionSnapshot effective = lastCompleteSnapshot;
    if (lastGenerationTimeMs <= 0.0 || now - lastGenerationTimeMs > staleDetectionMilliseconds)
    {
        effective.voiced = false;
        effective.frequencyHz = 0.0f;
        effective.confidence = 0.0f;
        effective.rms = 0.0f;
    }

    const auto result = stabilizer.update(effective, now);
    const std::uint16_t mask = result.pitchClass.has_value()
        ? static_cast<std::uint16_t>(1u << *result.pitchClass)
        : 0u;
    const bool heartbeat = now - lastHeartbeatMs >= heartbeatMilliseconds;
    sendMask(mask, heartbeat);
    if (heartbeat)
        lastHeartbeatMs = now;

    updatePublicDetection(result);
    publicStatus.store(static_cast<int>(result.valid ? Status::sending : Status::waitingForAudio),
                       std::memory_order_relaxed);
}

bool VrchatOscSender::acquireAndConnect()
{
    auto& ownership = SenderOwnership::instance();
    if (!ownership.isOwnedBy(this) && !ownership.tryAcquire(this))
    {
        owns.store(false, std::memory_order_relaxed);
        connectedState.store(false, std::memory_order_relaxed);
        publicStatus.store(static_cast<int>(Status::ownershipUnavailable), std::memory_order_relaxed);
        publicPitchClass.store(-1, std::memory_order_relaxed);
        return false;
    }

    owns.store(true, std::memory_order_relaxed);
    if (!socketConnected)
    {
        socketConnected = sender.connect(host, port);
        connectedState.store(socketConnected, std::memory_order_relaxed);
        lastSentMask = 0;
        lastHeartbeatMs = 0.0;
        stabilizer.clear();
        if (!socketConnected)
        {
            publicStatus.store(static_cast<int>(Status::connectionFailed), std::memory_order_relaxed);
            ownership.release(this);
            owns.store(false, std::memory_order_relaxed);
            return false;
        }
    }
    return true;
}

void VrchatOscSender::disableAndRelease()
{
    if (socketConnected)
    {
        sendMask(0u, true);
        sender.disconnect();
        socketConnected = false;
    }

    connectedState.store(false, std::memory_order_relaxed);
    if (SenderOwnership::instance().isOwnedBy(this))
        SenderOwnership::instance().release(this);
    owns.store(false, std::memory_order_relaxed);
    stabilizer.clear();
    lastGenerationTimeMs = 0.0;
    publicPitchClass.store(-1, std::memory_order_relaxed);
    publicFrequency.store(0.0f, std::memory_order_relaxed);
    publicConfidence.store(0.0f, std::memory_order_relaxed);
}

void VrchatOscSender::performOneShotClear()
{
    const bool alreadyContinuous = wantEnabled.load(std::memory_order_relaxed)
                                && SenderOwnership::instance().isOwnedBy(this);
    if (!SenderOwnership::instance().tryAcquire(this))
    {
        publicStatus.store(static_cast<int>(Status::ownershipUnavailable), std::memory_order_relaxed);
        return;
    }

    owns.store(true, std::memory_order_relaxed);
    if (!socketConnected)
    {
        socketConnected = sender.connect(host, port);
        connectedState.store(socketConnected, std::memory_order_relaxed);
    }

    if (socketConnected)
        sendMask(0u, true);

    stabilizer.clear();
    publicPitchClass.store(-1, std::memory_order_relaxed);

    if (!alreadyContinuous && !wantEnabled.load(std::memory_order_relaxed))
    {
        if (socketConnected)
            sender.disconnect();
        socketConnected = false;
        connectedState.store(false, std::memory_order_relaxed);
        SenderOwnership::instance().release(this);
        owns.store(false, std::memory_order_relaxed);
    }
}

void VrchatOscSender::sendMask(std::uint16_t mask, bool force)
{
    if (!socketConnected)
        return;

    std::uint16_t confirmed = lastSentMask;
    for (std::size_t index = 0; index < addresses.size(); ++index)
    {
        const auto bit = static_cast<std::uint16_t>(1u << index);
        const bool nowOn = (mask & bit) != 0u;
        const bool wasOn = (lastSentMask & bit) != 0u;
        if (!force && nowOn == wasOn)
            continue;

        juce::OSCMessage message(addresses[index]);
        message.addInt32(nowOn ? 1 : 0);
        if (sender.send(message))
        {
            if (nowOn)
                confirmed = static_cast<std::uint16_t>(confirmed | bit);
            else
                confirmed = static_cast<std::uint16_t>(confirmed & ~bit);
        }
    }
    lastSentMask = confirmed;
}

void VrchatOscSender::updatePublicDetection(const NoteStabilizer::Result& result) noexcept
{
    publicPitchClass.store(result.pitchClass.value_or(-1), std::memory_order_relaxed);
    publicFrequency.store(result.frequencyHz, std::memory_order_relaxed);
    publicConfidence.store(result.confidence, std::memory_order_relaxed);
}
} // namespace vrcnl
