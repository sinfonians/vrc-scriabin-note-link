# 設計と安全性契約 / Architecture and safety contract

## 日本語

### 製品境界

VRC Scriabin Note Linkは単音の基音周波数を検出し、12種類のピッチクラスのうち1つを対応VRChatアバターへ送ります。汎用VSTホスト、和音検出器、調検出器、録音機、音声処理機ではありません。

### 音声経路

- `FundamentalDetector` は事前確保済み履歴上で正規化自己相関を実行します。
- `prepareToPlay()` がモノラルscratch bufferと検出履歴を確保します。
- `processBlock()` は、メモリ確保、ロック、ソケット操作、ログ、ファイルI/O、prepare、ホスト通知を行いません。
- VST3は音声を変更せず、遅延0を報告します。
- Standaloneは解析後の出力を常に消去します。
- prepare済み容量を超えるブロックでは検出を安全側で停止し、Audio thread上で再確保しません。

### 検出結果の公開

Audio threadだけが `DetectionMailbox` へ書き込みます。atomic sequence counterの下でatomic scalar fieldsを公開します。読み取り側は最大32回再試行し、一致する偶数sequenceだけを採用します。失敗時は各読み取り側が保持する最後の完全なsnapshotを使用し、異なる世代の周波数、信頼度、RMS、有声状態が混在することを防ぎます。

### 音名の安定化

message threadで次を要求します。

- 70〜1000 Hz
- 信頼度0.50以上
- RMS 0.004以上（約-48 dBFS）
- 音名変更時は2回の確認
- dropout時は100 ms保持

最寄りのMIDI noteを `69 + 12 * log2(f / 440)` で計算し、12の剰余へ変換します。

### OSC境界

- 固定送信先: `127.0.0.1:9000`
- 固定アドレス: `/avatar/parameters/PC0`〜`/avatar/parameters/PC11`
- 固定値型: OSC int32の `0` または `1`
- 受信socketおよびリモート送信先設定なし
- 20 Hzのmessage-thread timerだけがsocket操作を実行
- 1秒ごとのheartbeatで12状態を再送
- Audio generationが150 ms停止すると失効し、bounded note hold後に全オフ
- 無効化、破棄、明示的All Offで12状態をbest-effort送信してオフ

### 送信者の所有権

process-global arbiterとWindows named interprocess lockにより、VST3／Standaloneを跨いで1つのNote Link送信者だけを許可します。他製品とは調停しないため、UIとREADMEではVRC Scriabin送信者を1つだけ有効にするよう求めます。

### ネットワークと状態の既定値

- 構築時および状態復元後は、継続OSCを必ずオフにします。
- All Offは継続送信も無効にし、生きた検出による即時再点灯を防ぎます。
- ネットワーク有効状態を保存しません。
- テレメトリー、アカウント、ブラウザー、更新機能、クラウド送信、録音、外部コマンド実行、第三者プラグイン読み込みはありません。

### リリース判定

自動テストは、周波数／ピッチクラス、倍音を含む入力、無音／ノイズ、heap allocation計測、mailbox整合性、OSC型／アドレス／lifecycle、古い検出、process内／process間所有権、processor透過性、Standalone無音、状態復元時の安全性を対象とします。ReleaseビルドはVST3とStandaloneの両方を生成します。実DAW、オーディオデバイス、画面表示、対応VRChatアバターは人間による確認項目です。

## English

### Product boundary

VRC Scriabin Note Link detects one monophonic fundamental frequency and sends one of twelve pitch classes to a compatible VRChat avatar. It is not a general VST host, chord detector, key detector, recorder, or voice processor.

### Audio path

- `FundamentalDetector` runs normalized autocorrelation over preallocated history.
- `prepareToPlay()` allocates the mono scratch buffer and detector history.
- `processBlock()` performs no allocation, lock, socket operation, logging, file I/O, preparation, or host notification.
- VST3 audio is not modified and reports zero latency.
- Standalone output is always cleared after analysis.
- A block larger than the prepared capacity fails silent for detection and never reallocates on the audio thread.

### Detection publication

The audio thread is the only writer to `DetectionMailbox`. It publishes atomic scalar fields under an atomic sequence counter. Readers retry at most 32 times and accept only matching even sequence values; otherwise, they keep their own last complete snapshot. This prevents mixed-generation frequency, confidence, RMS, and voiced state.

### Note stability

The message thread requires:

- 70 to 1000 Hz;
- confidence of at least 0.50;
- RMS of at least 0.004 (approximately -48 dBFS);
- two confirmations for a changed note; and
- a 100 ms dropout hold.

The nearest MIDI note is calculated from `69 + 12 * log2(f / 440)`, then reduced modulo 12.

### OSC boundary

- Fixed destination: `127.0.0.1:9000`.
- Fixed addresses: `/avatar/parameters/PC0` through `/avatar/parameters/PC11`.
- Fixed value type: OSC int32 `0` or `1`.
- No incoming socket and no remote destination setting.
- A 20 Hz message-thread timer performs all socket activity.
- A one-second heartbeat reasserts all twelve states.
- A stopped audio generation expires after 150 ms, then clears after the bounded note hold.
- Disable, destruction, and explicit All Off send all twelve states off on a best-effort basis.

### Sender ownership

A process-global arbiter and Windows named interprocess lock permit one Note Link sender owner across VST3 and Standalone instances. This does not arbitrate with unrelated products, so the UI and README require users to enable only one VRC Scriabin sender.

### Network and state defaults

- Continuous OSC is off after every construction and state restore.
- All Off is a separate explicit action that also disables continuous sending, so a live detection cannot immediately reactivate a pitch class.
- Network enable state is never persisted.
- The application has no telemetry, account, browser, updater, cloud upload, recording, external command execution, or third-party plug-in loading.

### Release gates

Automated tests cover frequency/pitch-class behavior, harmonic-rich inputs, silence/noise, heap-allocation instrumentation, mailbox consistency, OSC type/address/lifecycle, stale detection, in-process and interprocess ownership, processor transparency, Standalone silence, and safe state restoration. Release builds produce both VST3 and Standalone targets. Actual DAW behavior, audio-device behavior, visible layout, and a compatible avatar in VRChat remain human verification items.
