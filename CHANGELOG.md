# 更新履歴 / Changelog

## 1.0.0 - 2026-08-10

### 日本語

- オープンソース初回公開版。
- 70〜1000 Hzの単音基音検出。
- ループバック経由でVRChatへ `PC0`〜`PC11` を排他的にOSC送信。
- 音声を変更しないVST3経路と、出力を常に無音にするStandalone解析アプリ。
- OSC既定オフ、All Off、古い検出の失効、単一送信者所有権を実装。

### English

- Initial open-source release.
- Monophonic 70-1000 Hz fundamental detection.
- One-at-a-time `PC0` through `PC11` OSC output to VRChat on loopback.
- Transparent VST3 audio path and silent-output Standalone analyzer.
- Default-off networking, one-shot All Off, stale-note expiry, and single-instance sender ownership.
