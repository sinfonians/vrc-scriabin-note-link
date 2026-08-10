# VRC Scriabin Note Link

VRC Scriabin Note Link is a small Windows VST3 / Standalone tool that detects one monophonic fundamental frequency and sends its pitch class to a compatible VRChat avatar over OSC.

It does **not** process, record, upload, or transform your voice. The VST3 passes audio through unchanged. The Standalone build analyzes the selected input but always outputs silence, reducing feedback risk.

This is an unofficial community tool and is not affiliated with or endorsed by VRChat Inc.

## What it sends

The application sends OSC int32 values (`0` or `1`) to the fixed loopback destination `127.0.0.1:9000`:

```text
/avatar/parameters/PC0
/avatar/parameters/PC1
...
/avatar/parameters/PC11
```

Only one pitch class is active at a time. Silence or an unreliable detection clears all pitch classes after a short hold.

## Safety and privacy

- OSC is **off every time the plug-in or application starts**. Enable it explicitly when needed.
- The destination is fixed to the local computer. There is no incoming network server or remote-IP setting.
- There is no telemetry, account, cloud upload, browser, updater, plug-in scanner, or recording function.
- Run only one application that sends the VRC Scriabin `PC0` through `PC11` parameters. Note Link prevents its own instances from sending together, but it cannot coordinate with unrelated senders.
- Press **All Off** to disable continuous OSC and clear all twelve parameters. This works even when continuous OSC was already off.

## Installation

### VST3

Copy the `VRC Scriabin Note Link.vst3` bundle to your normal VST3 location, commonly:

```text
C:\Program Files\Common Files\VST3
```

Rescan plug-ins in your DAW, insert it on the live input track, and enable OSC in the editor.

### Standalone

Run `VRC Scriabin Note Link.exe`, choose the microphone/input device in the audio settings, then enable OSC.

The Standalone output is intentionally silent. It is a detector and OSC sender, not an audio monitor.

## VRChat setup

1. Enable OSC in VRChat.
2. Use an avatar compatible with the VRC Scriabin `PC0` through `PC11` contract.
3. Disable ASMRForm, ASMRKey, or any other application sending those same parameters.
4. Start Note Link, verify the intended input meter moves, then enable OSC.
5. Sing or play one clear note at a time.

After an abnormal termination, reopen the Standalone application and press **All Off**, or restart VRChat OSC.

## Windows download warning

The first public build may be unsigned. Windows Defender SmartScreen can show “Windows protected your PC” for unsigned or low-reputation downloads. Download only from this repository’s official GitHub Release, compare the published SHA-256 checksums, and inspect/build the source if you are unsure. Public source and checksums improve verifiability but do not replace publisher code signing.

## Build

Requirements:

- Windows 10/11
- Visual Studio 2022 with Desktop development with C++
- CMake 3.22 or newer
- Git

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

JUCE 8.0.6 is fetched during configure at the exact commit recorded in `CMakeLists.txt`.

## Maintainer release packaging

`tools/package-release.ps1` intentionally permits staging only below `P:\_tmp`. This protects the source checkout and prevents accidental replacement of unrelated files. Override `-BuildRoot` when using a non-default build directory; do not change the staging boundary without a separate safety review.

## Scope

Version 1 intentionally includes only monophonic fundamental detection and twelve pitch-class OSC outputs. It does not detect chords, infer a key or scale, load VST plug-ins, or modify voice/audio.

## License

VRC Scriabin Note Link is licensed under the GNU Affero General Public License v3.0 or later. See [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
