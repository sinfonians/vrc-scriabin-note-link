# コントリビューション / Contributing

## 日本語

VRC Scriabin Note Linkの改善へのご協力ありがとうございます。

1. 変更は、単音基音検出と固定された12種類のピッチクラスOSC出力という対象範囲内に保ってください。
2. 別途設計レビューを行わず、テレメトリー、リモート通信、録音、プラグイン読み込み、アカウント、クラウドサービスを追加しないでください。
3. Audio thread上で、メモリ確保、ロック、ファイルI/O、ログ出力、ソケット操作を行わないでください。
4. 挙動変更には決定論的なテストを追加してください。
5. Pull Requestを作る前にReleaseビルドとCTestを実行してください。
6. 録音した音声、個人情報、製品パッケージ、認証情報、非公開仕様を提出しないでください。

コントリビューションには本リポジトリのAGPL-3.0-or-laterライセンスが適用されます。

## English

Thank you for helping improve VRC Scriabin Note Link.

1. Keep changes inside the stated scope: monophonic fundamental detection and the fixed twelve pitch-class OSC outputs.
2. Do not add telemetry, remote networking, recording, plug-in loading, accounts, or cloud services without a separately reviewed design.
3. Do not allocate, lock, perform file I/O, log, or use sockets on the audio thread.
4. Add deterministic tests for behavior changes.
5. Run the Release build and CTest suite before opening a pull request.
6. Do not submit captured voice, personal data, product packages, credentials, or private specifications.

Contributions are accepted under the repository's AGPL-3.0-or-later license.
