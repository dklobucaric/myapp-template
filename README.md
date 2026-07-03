# MyAppTemplate

Reusable cross-platform desktop application template built with C++20, Qt 6 Widgets and CMake.

MyAppTemplate provides the reusable application shell around future desktop products:

- Menu Bar
- Main Content Area
- Status Bar
- Settings system
- JSON configuration
- License import and validation foundation
- CDN/update manifest foundation
- Diagnostics and structured logging foundation
- Test structure
- Build, release and deployment tooling

## Development status

Current version: `0.1.0`

Primary development platform: Linux

Windows and macOS will receive early smoke-test builds before platform-specific release work begins.

## Quick start

Documentation will be added under [`docs/`](docs/).

The future application workspace lives under:

```text
src/workspace/
src/app/
src/core/
src/ui/
src/settings/
src/licensing/
src/updates/
src/platform/

##Repository purpose

This repository is a template. Future applications should be created from it and then customize:

- application name
- product ID
- branding and icons
- feature flags
- service endpoints
- Main Content Area
