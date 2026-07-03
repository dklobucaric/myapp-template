# MyAppTemplate

Reusable cross-platform desktop application template built with C++20, Qt 6
Widgets and CMake.

MyAppTemplate provides the reusable application shell around future desktop
products:

- Menu Bar
- Main Content Area
- Status Bar
- Functional Settings system with Apply, Cancel, Reset Page and Reset All
- Layered JSON configuration with development, staging and production profiles
- Theme, accent color, font scale and display preferences
- Server-authoritative LicenseManager foundation
- CDN/update manifest foundation
- Diagnostics and structured logging foundation
- Test structure
- Build, release and deployment tooling

## Development status

Current version: `0.1.5`

Primary development platform: Linux.

Windows and macOS will receive early smoke-test builds before platform-specific
release work begins.

## Quick start

```bash
./scripts/build.sh debug
./build/linux-debug/myapp-template
```

Run automated tests:

```bash
./scripts/test.sh debug
```

More detailed instructions are available under [`docs/`](docs/).

## Functional Settings

Open **Settings → Settings…** to change application preferences. Settings are
saved as a small local override layer, so switching environment profiles still
uses the correct development, staging or production defaults unless the user
explicitly overrides a value.

See [`docs/SETTINGS.md`](docs/SETTINGS.md) for the complete behavior.

## Updates

**Help → Check for Updates** checks the active mock or configured CDN manifest,
updates the Dashboard and Status Bar, and persists a redacted local check state.
It does not download or install an artifact yet. See [`docs/UPDATES.md`](docs/UPDATES.md).

## License status

**Help → Check License Status** checks the configured license service and
shows the server-authoritative entitlement state, plan duration, redacted
serial suffix, device usage and expiry. The server—not the serial—controls
seat limits and renewal duration. See [`docs/LICENSE_SERVER.md`](docs/LICENSE_SERVER.md).

## Logging and diagnostics

The **Diagnostics** Settings page controls local structured logging, opens the
local logs folder and creates a safe support ZIP. The app never uploads a
support package automatically. See [`docs/DIAGNOSTICS.md`](docs/DIAGNOSTICS.md).

## Where to start a new application

The product-specific Main Content Area lives under:

```text
src/workspace/
```

The reusable application shell lives under:

```text
src/app/
src/core/
src/ui/
src/settings/
src/licensing/
src/updates/
src/platform/
```

## Repository purpose

This repository is a template. Future applications should be created from it
and then customize:

- application name
- product ID
- branding and icons
- feature flags
- service endpoints
- Main Content Area
