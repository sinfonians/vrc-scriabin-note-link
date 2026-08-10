// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SenderOwnership.h"

namespace vrcnl
{
SenderOwnership& SenderOwnership::instance()
{
    static SenderOwnership ownership;
    return ownership;
}

SenderOwnership::SenderOwnership()
    : processLock("com.asimosound.vrcscriabinnote.osc-sender")
{
}

bool SenderOwnership::tryAcquire(const void* owner)
{
    if (owner == nullptr)
        return false;

    const std::lock_guard<std::mutex> lock(guard);
    if (currentOwner == owner)
        return true;
    if (currentOwner != nullptr)
        return false;
    if (!processLock.enter(0))
        return false;

    currentOwner = owner;
    return true;
}

void SenderOwnership::release(const void* owner)
{
    const std::lock_guard<std::mutex> lock(guard);
    if (currentOwner != owner)
        return;

    currentOwner = nullptr;
    processLock.exit();
}

bool SenderOwnership::isOwnedBy(const void* owner) const
{
    const std::lock_guard<std::mutex> lock(guard);
    return currentOwner == owner;
}
} // namespace vrcnl
