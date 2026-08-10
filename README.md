# VRC Scriabin Note Link

[日本語](#日本語) | [English](#english)

## 日本語

VRC Scriabin Note Linkは、単音の基音周波数を検出し、その音名（ピッチクラス）をOSCで対応VRChatアバターへ送る、小さなWindows用VST3／Standaloneツールです。

声の加工、録音、アップロード、変換は行いません。VST3版は音声を変更せず、そのまま通過させます。Standalone版は選択した入力を解析しますが、ハウリング防止のため音声出力は常に無音です。

本ツールは非公式のコミュニティ製ツールであり、VRChat Inc.との提携・承認関係はありません。

### 送信内容

固定されたローカル宛先 `127.0.0.1:9000` へ、OSC int32値（`0` または `1`）を送信します。

```text
/avatar/parameters/PC0
/avatar/parameters/PC1
...
/avatar/parameters/PC11
```

同時に有効になるピッチクラスは1つだけです。無音または信頼できない検出が続くと、短い保持時間の後に全ピッチクラスをオフにします。

### 安全性とプライバシー

- プラグイン／アプリの起動時は、OSCが毎回**オフ**になります。必要なときだけ明示的に有効化してください。
- 送信先は同じPC内に固定されています。受信用ネットワークサーバーやリモートIP設定はありません。
- テレメトリー、アカウント、クラウド送信、ブラウザー、更新機能、プラグインスキャン、録音機能はありません。
- VRC Scriabinの `PC0`〜`PC11` を送るアプリは1つだけ有効にしてください。本ツール同士の多重送信は防止しますが、他製品の送信状態までは調停できません。
- **All Off（すべてオフ）**を押すと、継続送信を無効化して12パラメータをすべてオフにします。OSC継続送信が既にオフでも実行できます。

### インストール

#### VST3

`VRC Scriabin Note Link.vst3` フォルダーを、通常のVST3保存先へコピーします。一般的な場所は次のとおりです。

```text
C:\Program Files\Common Files\VST3
```

DAWでプラグインを再スキャンし、ライブ入力トラックへ挿入して、エディター上でOSCを有効にします。

#### Standalone

`VRC Scriabin Note Link.exe` を起動し、オーディオ設定でマイク／入力デバイスを選択してからOSCを有効にします。

Standalone版の音声出力は意図的に無音です。音声モニターではなく、検出・OSC送信用アプリです。

### VRChatでの使用

1. VRChatのOSCを有効にします。
2. VRC Scriabinの `PC0`〜`PC11` 契約に対応したアバターを使用します。
3. ASMRForm、ASMRKeyなど、同じパラメータを送る他アプリを無効にします。
4. Note Linkを起動し、目的の入力でメーターが動くことを確認してからOSCを有効にします。
5. 一度に1つの明瞭な音を歌う、または演奏します。

異常終了後はStandalone版を再度開いて **All Off** を押すか、VRChat OSCを再起動してください。

### Windowsのダウンロード警告

公開バイナリは未署名です。Windows Defender SmartScreenが「WindowsによってPCが保護されました」と表示する場合があります。本リポジトリの公式GitHub Releaseからのみダウンロードし、公開SHA-256と照合してください。不安がある場合はソースコードを確認してビルドしてください。公開ソースとチェックサムは検証可能性を高めますが、発行元コード署名の代わりではありません。

### ビルド

必要環境:

- Windows 10／11
- Visual Studio 2022（C++によるデスクトップ開発）
- CMake 3.22以降
- Git

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

構成時に、`CMakeLists.txt` に記録された正確なコミットのJUCE 8.0.6を取得します。

### メンテナー向けパッケージ作成

`tools/package-release.ps1` は、誤操作防止のため `P:\_tmp` 配下だけをステージング先として許可します。既定外のビルドディレクトリを使う場合は `-BuildRoot` を指定してください。別途安全性を検討せず、この境界を変更しないでください。

### 対象範囲

Version 1は、単音の基音検出と12種類のピッチクラスOSC出力だけを提供します。和音検出、調・音階の推定、VSTプラグインの読み込み、音声加工は行いません。

### ライセンス

GNU Affero General Public License v3.0以降（AGPL-3.0-or-later）で公開しています。[LICENSE](LICENSE) と [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) を確認してください。法的なライセンス条件は同梱の英語原文が正式です。

## English

VRC Scriabin Note Link is a small Windows VST3 / Standalone tool that detects one monophonic fundamental frequency and sends its pitch class to a compatible VRChat avatar over OSC.

It does **not** process, record, upload, or transform your voice. The VST3 passes audio through unchanged. The Standalone build analyzes the selected input but always outputs silence, reducing feedback risk.

This is an unofficial community tool and is not affiliated with or endorsed by VRChat Inc.

### What it sends

The application sends OSC int32 values (`0` or `1`) to the fixed loopback destination `127.0.0.1:9000`:

```text
/avatar/parameters/PC0
/avatar/parameters/PC1
...
/avatar/parameters/PC11
```

Only one pitch class is active at a time. Silence or an unreliable detection clears all pitch classes after a short hold.

### Safety and privacy

- OSC is **off every time the plug-in or application starts**. Enable it explicitly when needed.
- The destination is fixed to the local computer. There is no incoming network server or remote-IP setting.
- There is no telemetry, account, cloud upload, browser, updater, plug-in scanner, or recording function.
- Run only one application that sends the VRC Scriabin `PC0` through `PC11` parameters. Note Link prevents its own instances from sending together, but it cannot coordinate with unrelated senders.
- Press **All Off** to disable continuous OSC and clear all twelve parameters. This works even when continuous OSC was already off.

### Installation

#### VST3

Copy the `VRC Scriabin Note Link.vst3` bundle to your normal VST3 location, commonly:

```text
C:\Program Files\Common Files\VST3
```

Rescan plug-ins in your DAW, insert it on the live input track, and enable OSC in the editor.

#### Standalone

Run `VRC Scriabin Note Link.exe`, choose the microphone/input device in the audio settings, then enable OSC.

The Standalone output is intentionally silent. It is a detector and OSC sender, not an audio monitor.

### VRChat setup

1. Enable OSC in VRChat.
2. Use an avatar compatible with the VRC Scriabin `PC0` through `PC11` contract.
3. Disable ASMRForm, ASMRKey, or any other application sending those same parameters.
4. Start Note Link, verify the intended input meter moves, then enable OSC.
5. Sing or play one clear note at a time.

After an abnormal termination, reopen the Standalone application and press **All Off**, or restart VRChat OSC.

### Windows download warning

The public binaries are unsigned. Windows Defender SmartScreen can show “Windows protected your PC” for unsigned or low-reputation downloads. Download only from this repository’s official GitHub Release, compare the published SHA-256 checksums, and inspect/build the source if you are unsure. Public source and checksums improve verifiability but do not replace publisher code signing.

### Build

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

### Maintainer release packaging

`tools/package-release.ps1` intentionally permits staging only below `P:\_tmp`. This protects the source checkout and prevents accidental replacement of unrelated files. Override `-BuildRoot` when using a non-default build directory; do not change the staging boundary without a separate safety review.

### Scope

Version 1 intentionally includes only monophonic fundamental detection and twelve pitch-class OSC outputs. It does not detect chords, infer a key or scale, load VST plug-ins, or modify voice/audio.

### License

VRC Scriabin Note Link is licensed under the GNU Affero General Public License v3.0 or later. See [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The included English license texts are legally authoritative.
