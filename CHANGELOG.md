# Changelog

All notable changes to this project are documented in this file.

## Unreleased

### Fixed
- Enable CMake AUTORCC so built-in JSON resources are compiled into each target.


- Explicitly initialize Qt built-in JSON resources before configuration loading.
- Add a ConfigManager test that verifies embedded configuration resources are available.

## [0.1.1] - 2026-07-03

### Added

- `ConfigManager` with layered JSON configuration loading.
- Qt resources for the app profile, default config and environment profiles.
- Runtime AppData `config.json` creation using atomic `QSaveFile` writes.
- `--profile` command-line option for development, staging and production.
- Dynamic Template Dashboard values loaded from configuration.
- Read-only Settings Dialog shell with the agreed settings categories.
- ConfigManager unit tests and `scripts/test.sh`.

### Changed

- Version metadata now comes from `VERSION` and `app-profile.json`.
- MainWindow title, window size, Status Bar visibility and endpoint displays
  now use runtime configuration rather than hardcoded strings.
