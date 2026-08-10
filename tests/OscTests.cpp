// Copyright (C) 2026 asimosound
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Core/DetectionMailbox.h"
#include "Osc/SenderOwnership.h"
#include "Osc/VrchatOscSender.h"

#include <juce_events/juce_events.h>
#include <juce_osc/juce_osc.h>

#include <array>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace
{
constexpr int testPort = 19090;
int failures = 0;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

struct Packet
{
    juce::String address;
    int value = -1;
};

class Receiver final : private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    Receiver()
    {
        connected = receiver.connect(testPort);
        if (connected)
            receiver.addListener(this);
    }

    ~Receiver() override
    {
        if (connected)
        {
            receiver.removeListener(this);
            receiver.disconnect();
        }
    }

    bool isConnected() const noexcept { return connected; }
    void clear() { packets.clear(); }

    bool saw(const juce::String& address, int value) const
    {
        for (const auto& packet : packets)
            if (packet.address == address && packet.value == value)
                return true;
        return false;
    }

    int countOnMessages() const
    {
        int count = 0;
        for (const auto& packet : packets)
            if (packet.value == 1)
                ++count;
        return count;
    }

private:
    void oscMessageReceived(const juce::OSCMessage& message) override
    {
        if (message.size() != 1 || !message[0].isInt32())
        {
            packets.push_back({ message.getAddressPattern().toString(), -2 });
            return;
        }
        packets.push_back({ message.getAddressPattern().toString(), message[0].getInt32() });
    }

    juce::OSCReceiver receiver;
    bool connected = false;
    std::vector<Packet> packets;
};

void dispatch(int milliseconds)
{
    juce::MessageManager::getInstance()->runDispatchLoopUntil(milliseconds);
}

bool waitUntil(const std::function<bool()>& condition, int timeoutMs)
{
    const double deadline = juce::Time::getMillisecondCounterHiRes() + timeoutMs;
    while (juce::Time::getMillisecondCounterHiRes() < deadline)
    {
        if (condition())
            return true;
        dispatch(25);
    }
    return condition();
}

int runOwnershipChild(bool expectBlocked)
{
    int childOwner = 0;
    const bool acquired = vrcnl::SenderOwnership::instance().tryAcquire(&childOwner);
    if (acquired)
        vrcnl::SenderOwnership::instance().release(&childOwner);
    return acquired == !expectBlocked ? 0 : 20;
}

bool runChild(const juce::String& argument)
{
    const auto executable = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName();
    juce::ChildProcess child;
    if (!child.start(executable + " " + argument))
        return false;
    if (!child.waitForProcessToFinish(10000))
        return false;
    return child.getExitCode() == 0;
}

void testOwnership()
{
    int ownerA = 0;
    int ownerB = 0;
    auto& ownership = vrcnl::SenderOwnership::instance();
    require(ownership.tryAcquire(&ownerA), "first owner must acquire sender");
    require(!ownership.tryAcquire(&ownerB), "second in-process owner must be rejected");
    require(runChild("--expect-ownership-blocked"), "second process must be rejected while lock is owned");
    ownership.release(&ownerA);
    require(ownership.tryAcquire(&ownerB), "ownership must transfer after release");
    ownership.release(&ownerB);
    require(runChild("--expect-ownership-acquired"), "second process must acquire after lock release");
}

void testOscLifecycle()
{
    Receiver receiver;
    require(receiver.isConnected(), "test OSC receiver must bind loopback port");
    if (!receiver.isConnected())
        return;

    vrcnl::DetectionMailbox mailbox;
    vrcnl::VrchatOscSender sender(mailbox, "127.0.0.1", testPort);
    dispatch(80);
    require(receiver.countOnMessages() == 0, "OSC must default off");

    mailbox.publish({ 440.0f, 0.95f, 0.1f, true, 1 });
    sender.setEnabled(true);
    require(waitUntil([&] { return receiver.saw("/avatar/parameters/PC9", 1); }, 800),
            "A4 activation must arrive before deadline");
    require(sender.ownsSender(), "enabled sender must own endpoint");
    require(receiver.saw("/avatar/parameters/PC9", 1), "A4 must send PC9 int32=1");
    require(receiver.countOnMessages() == 1, "only one pitch class may be on");

    receiver.clear();
    mailbox.publish({ 0.0f, 0.0f, 0.0f, false, 2 });
    require(waitUntil([&] { return receiver.saw("/avatar/parameters/PC9", 0); }, 800),
            "silence clear must arrive before deadline");
    require(receiver.saw("/avatar/parameters/PC9", 0), "sustained silence must send PC9 off");

    receiver.clear();
    mailbox.publish({ 440.0f, 0.95f, 0.1f, true, 3 });
    require(waitUntil([&] { return receiver.saw("/avatar/parameters/PC9", 1); }, 800),
            "fresh generation activation must arrive before deadline");
    require(receiver.saw("/avatar/parameters/PC9", 1), "fresh audio generation must reactivate PC9");
    receiver.clear();
    require(waitUntil([&] { return receiver.saw("/avatar/parameters/PC9", 0); }, 1000),
            "stale clear must arrive before deadline");
    require(receiver.saw("/avatar/parameters/PC9", 0), "stale audio generation must expire to all-off");

    receiver.clear();
    sender.setEnabled(false);
    require(waitUntil([&] { return receiver.saw("/avatar/parameters/PC0", 0); }, 500),
            "disable all-off must arrive before deadline");
    require(!sender.ownsSender(), "disabled sender must release ownership");
    for (int pitchClass = 0; pitchClass < 12; ++pitchClass)
        require(receiver.saw("/avatar/parameters/PC" + juce::String(pitchClass), 0),
                "disable must force every pitch class off");

    receiver.clear();
    sender.setEnabled(true);
    sender.requestAllOff();
    require(waitUntil([&] { return receiver.saw("/avatar/parameters/PC0", 0); }, 500),
            "one-shot all-off must arrive before deadline");
    for (int pitchClass = 0; pitchClass < 12; ++pitchClass)
        require(receiver.saw("/avatar/parameters/PC" + juce::String(pitchClass), 0),
                "one-shot All Off must send every pitch class off");
    require(!sender.ownsSender(), "one-shot All Off must release temporary ownership");
    require(!sender.enabled(), "All Off must also disable continuous OSC");
}
} // namespace

int main(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--expect-ownership-blocked")
        return runOwnershipChild(true);
    if (argc == 2 && std::string(argv[1]) == "--expect-ownership-acquired")
        return runOwnershipChild(false);

    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    testOwnership();
    testOscLifecycle();

    if (failures == 0)
    {
        std::cout << "All OSC contracts passed.\n";
        return 0;
    }
    std::cerr << failures << " OSC contract(s) failed.\n";
    return 1;
}
