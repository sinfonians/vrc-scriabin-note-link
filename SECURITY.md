# セキュリティ方針 / Security Policy

## 日本語

### 対応バージョン

セキュリティ修正は、公開済みの最新版を対象に提供します。

### 脆弱性の報告

公開Issueへ、秘密情報、個人情報、録音音声、非公開アバターファイルを添付しないでください。

本リポジトリのGitHub非公開脆弱性報告機能を使用してください。この機能が利用できない場合は、機密を含まない最小限の説明だけを公開Issueへ記載し、非公開の連絡経路を依頼してください。

### ネットワーク境界

本アプリは `127.0.0.1:9000` へ固定OSCメッセージだけを送信します。外部からの接続を待ち受けず、リモート送信先も設定できません。

## English

### Supported versions

Security fixes are provided for the latest published release.

### Reporting a vulnerability

Do not include secrets, personal information, captured audio, or private avatar files in a public issue.

Use GitHub's private vulnerability reporting feature for this repository. If that feature is unavailable, open a public issue containing only a minimal, non-sensitive description and request a private contact channel.

### Network boundary

The application sends fixed OSC messages only to `127.0.0.1:9000`. It does not listen for incoming connections and does not support remote destinations.
