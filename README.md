# MyAppTemplate

Reusable cross-platform desktop application template built with C++20, Qt 6
Widgets and CMake.

MyAppTemplate provides the reusable application shell around future desktop
products:

- Menu Bar
- Main Content Area
- Status Bar
- Settings system
- Layered JSON configuration with development, staging and production profiles
- License import and validation foundation
- CDN/update manifest foundation
- Diagnostics and structured logging foundation
- Test structure
- Build, release and deployment tooling

## Development status

Current version: `0.1.1`

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
