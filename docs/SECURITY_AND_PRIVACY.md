# Security and privacy

## Local diagnostics

Logging and diagnostics are local-only. A log record is filtered by the configured
level, sanitized before it is written, and stored under the application AppData
directory. The sanitizer masks common password, token, API-key, authorization,
serial and license-key shaped values, URL query strings and the current home path.

A Safe Support Package is created as a local ZIP and is never uploaded by the
application. It contains a runtime summary, a deliberately limited safe config
summary and no more than the latest 200 redacted log lines. It does not include
`config.json`, license files, private keys, passwords, tokens, serials or full
home-directory paths. Review every package before sharing it.
