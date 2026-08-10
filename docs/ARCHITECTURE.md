# Architecture and safety contract

## Product boundary

VRC Scriabin Note Link detects one monophonic fundamental frequency and sends one of twelve pitch classes to a compatible VRChat avatar. It is not a general VST host, chord detector, key detector, recorder, or voice processor.

## Audio path

- `FundamentalDetector` runs normalized autocorrelation over preallocated history.
- `prepareToPlay()` allocates the mono scratch buffer and detector history.
- `processBlock()` performs no allocation, lock, socket operation, logging, file I/O, preparation, or host notification.
- VST3 audio is not modified and reports zero latency.
- Standalone output is always cleared after analysis.
- A block larger than the prepared capacity fails silent for detection and never reallocates on the audio thread.

## Detection publication

The audio thread is the only writer to `DetectionMailbox`. It publishes atomic scalar fields under an atomic sequence counter. Readers retry at most 32 times and accept only matching even sequence values; otherwise, they keep their own last complete snapshot. This prevents mixed-generation frequency, confidence, RMS, and voiced state.

## Note stability

The message thread requires:

- 70 to 1000 Hz;
- confidence of at least 0.50;
- RMS of at least 0.004 (approximately -48 dBFS);
- two confirmations for a changed note; and
- a 100 ms dropout hold.

The nearest MIDI note is calculated from `69 + 12 * log2(f / 440)`, then reduced modulo 12.

## OSC boundary

- Fixed destination: `127.0.0.1:9000`.
- Fixed addresses: `/avatar/parameters/PC0` through `/avatar/parameters/PC11`.
- Fixed value type: OSC int32 `0` or `1`.
- No incoming socket and no remote destination setting.
- A 20 Hz message-thread timer performs all socket activity.
- A one-second heartbeat reasserts all twelve states.
- A stopped audio generation expires after 150 ms, then clears after the bounded note hold.
- Disable, destruction, and explicit All Off send all twelve states off on a best-effort basis.

## Sender ownership

A process-global arbiter and Windows named interprocess lock permit one Note Link sender owner across VST3 and Standalone instances. This does not arbitrate with unrelated products, so the UI and README require users to enable only one VRC Scriabin sender.

## Network and state defaults

- Continuous OSC is off after every construction and state restore.
- All Off is a separate explicit action that also disables continuous sending, so a live detection cannot immediately reactivate a pitch class.
- Network enable state is never persisted.
- The application has no telemetry, account, browser, updater, cloud upload, recording, external command execution, or third-party plug-in loading.

## Release gates

Automated tests cover frequency/pitch-class behavior, harmonic-rich inputs, silence/noise, heap-allocation instrumentation, mailbox consistency, OSC type/address/lifecycle, stale detection, in-process and interprocess ownership, processor transparency, Standalone silence, and safe state restoration. Release builds produce both VST3 and Standalone targets. Actual DAW behavior, audio-device behavior, visible layout, and a compatible avatar in VRChat remain human verification items.
