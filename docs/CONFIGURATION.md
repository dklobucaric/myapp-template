# Configuration

MyAppTemplate uses JSON configuration layers so future applications can change
identity, defaults, endpoints and user preferences without scattering values
through the C++ source tree.

## Files

| File | Purpose |
| --- | --- |
| `assets/defaults/app-profile.json` | Product identity and product defaults compiled into the app resources. |
| `assets/defaults/config.json` | Built-in baseline configuration compiled into the app resources. |
| `profiles/development.json` | Local development service endpoints and development update channel. |
| `profiles/staging.json` | Test-server service endpoints and beta update channel. |
| `profiles/production.json` | Production service endpoints and stable update channel. |
| User `config.json` | User-specific runtime overrides stored in AppData. |

## Configuration precedence

The highest layer wins:

```text
default config
→ app-profile defaults
→ environment profile
→ user config.json
```

Environment profiles select service endpoints and update channels. User config
contains only explicit local overrides, so switching from development to
staging still uses staging endpoints until the user deliberately overrides a
field.

## Runtime user config location

The application creates the local file on first start with empty override
sections. Add only the values that should override the built-in and
environment defaults.

```text
Linux:
~/.local/share/DD-Lab/MyAppTemplate/config.json

Windows:
%LOCALAPPDATA%/DD-Lab/MyAppTemplate/config.json

macOS:
~/Library/Application Support/DD-Lab/MyAppTemplate/config.json
```

The exact location is visible in Settings → General and Settings → Advanced.

## Environment selection

The default profile is set by `app-profile.json`.

Use a different profile while testing:

```bash
./build/linux-debug/myapp-template --profile development
./build/linux-debug/myapp-template --profile staging
./build/linux-debug/myapp-template --profile production
```

Supported profile names are `development`, `staging` and `production`.

## Functional Settings

The Settings Dialog persists values as local overrides. It compares every
value against the active profile baseline. When a value equals the baseline,
that override is removed from local `config.json` rather than redundantly
stored.

This gives reset behavior the expected result:

```text
user override removed
→ active environment profile becomes visible again
```

See [`SETTINGS.md`](SETTINGS.md) for each page and its behavior.

## License check preferences

The `licensing` section contains `automaticChecks` and
`checkIntervalMinutes`. These are local scheduling preferences. A valid server
response can provide `nextOnlineCheckAfter`, which takes precedence for the
next background verification.

## Safe writes

`ConfigManager` writes local JSON through `QSaveFile`. The file is written to a
temporary location and committed atomically so a crash or power loss does not
leave a half-written config file.

Malformed local JSON is never overwritten automatically. The app logs a
configuration warning, ignores that malformed override for the current run and
continues with the safe built-in and environment defaults.

## Built-in resources

`app-profile.json`, the default configuration and environment profiles are
compiled into the application through the Qt resource system. `CMAKE_AUTORCC`
compiles the resource collection into both the application and unit-test
binaries, so `ConfigManager` can load `:/defaults/...` and `:/profiles/...`
without manual resource initialization.
