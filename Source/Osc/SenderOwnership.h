// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_core/juce_core.h>

#include <mutex>

namespace vrcnl
{
class SenderOwnership
{
public:
    static SenderOwnership& instance();

    bool tryAcquire(const void* owner);
    void release(const void* owner);
    [[nodiscard]] bool isOwnedBy(const void* owner) const;

private:
    SenderOwnership();

    mutable std::mutex guard;
    juce::InterProcessLock processLock;
    const void* currentOwner = nullptr;
};
} // namespace vrcnl
