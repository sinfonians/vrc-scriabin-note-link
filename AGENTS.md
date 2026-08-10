# Development rules

- Keep version 1 limited to monophonic fundamental detection and fixed `PC0` through `PC11` loopback OSC output.
- Never allocate, lock, prepare DSP, perform socket/file I/O, log, or notify the host on the audio thread.
- OSC must default off after construction and state restoration.
- The VST3 audio path must remain sample-transparent with zero reported latency.
- The Standalone output must remain silent unless a separately reviewed version changes that contract.
- Parameter/state identifiers are append-only once published.
- Do not add telemetry, accounts, cloud upload, incoming networking, editable remote targets, recording, or third-party plug-in loading without a security review.
- Commit, push, binary packaging, signing, and publication are separately reviewable operations.
