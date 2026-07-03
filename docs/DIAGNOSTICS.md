# Diagnostics

## Local logging

`AppLogger` writes local structured lines to the application AppData directory:

```text
<app-data>/logs/app.log
```

The **Diagnostics** Settings page controls whether logging is enabled and the
minimum level: `debug`, `info`, `warning` or `error`. The current log rotates at
512 KiB and the logger keeps at most four archive files (`app.1.log` through
`app.4.log`).

Before a message reaches disk, the logger masks common password, token, API key,
authorization, serial and license-key shaped values. It also replaces the
current home path and app-data path with placeholders, and strips URL query
strings because signed URLs often contain credentials.

## Safe Support Package

Choose **Settings → Diagnostics → Create Safe Support Package** to create a
local ZIP in:

```text
<app-data>/diagnostics/
```

The app never uploads this file. The ZIP uses standard uncompressed ZIP records
for broad platform compatibility and contains only:

| File | Contents |
| --- | --- |
| `README.txt` | What the package contains and what it excludes. |
| `runtime-summary.txt` | App version, product ID, environment, update channel and platform details. |
| `config-summary.json` | Limited, redacted runtime/service settings; never a copy of `config.json`. |
| `log-excerpt.txt` | Up to the latest 200 redacted lines from `app.log`. |

The package does **not** include `config.json`, license files, private keys,
passwords, tokens, serial values or a complete home-directory path. Always
inspect a support package before sharing it.
