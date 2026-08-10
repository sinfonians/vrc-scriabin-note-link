# Security Policy

## Supported versions

Security fixes are provided for the latest published release.

## Reporting a vulnerability

Do not include secrets, personal information, captured audio, or private avatar files in a public issue.

Until a dedicated security-reporting address is published, use GitHub's private vulnerability reporting feature for this repository. If that feature is unavailable, open a public issue containing only a minimal, non-sensitive description and request a private contact channel.

## Network boundary

The application sends fixed OSC messages only to `127.0.0.1:9000`. It does not listen for incoming connections and does not support remote destinations.
