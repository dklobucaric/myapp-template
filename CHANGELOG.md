# Changelog

All notable changes to this project are documented in this file.

## Unreleased

## [0.1.3] - 2026-07-03

### Added

- `AppLogger` with local structured logging, level filtering, redaction and
  bounded log rotation.
- `DiagnosticsManager` with local safe support ZIP creation.
- Diagnostics Settings actions to open the logs folder and create a support
  package without automatic upload.
- Logging and diagnostics automated tests covering disabled logging, filtering,
  redaction, rotation and safe-package content.
- `docs/DIAGNOSTICS.md` with package contents and privacy guarantees.

### Changed

- Template Dashboard now shows the current logs directory and diagnostics
  package availability.
- Mock update release advanced to `0.1.4` for the v0.1.3 application build.

## [0.1.2] - 2026-07-03

### Added

- Functional Settings Dialog with Apply, OK, Cancel, Reset Current Page and
  Reset All Settings actions.
- Live appearance controls for System, Light and Dark themes, accent color and
  font scale.
- Layout controls for startup dimensions and Status Bar visibility.
- Persisted update, CDN, license endpoint and diagnostics preferences.
- `ThemeManager` for reversible application palette and font application.
- ConfigManager support for baseline-aware preview and reset behavior.
- Additional ConfigManager tests for override clearing and preview behavior.
- Settings reference documentation and functional manual test checklist.

### Changed

- Template Dashboard and Status Bar now refresh after applied Settings changes.
- Mock update release advanced to `0.1.3` for the v0.1.2 application build.

## [0.1.1] - 2026-07-03

### Added

- `ConfigManager` with layered JSON configuration loading.
- Qt resources for the app profile, default config and environment profiles.
- Runtime AppData `config.json` creation using atomic `QSaveFile` writes.
- `--profile` command-line option for development, staging and production.
- Dynamic Template Dashboard values loaded from configuration.
- Settings Dialog shell with the agreed settings categories.
- ConfigManager unit tests and `scripts/test.sh`.

### Fixed

- Enable CMake AUTORCC so built-in JSON resources are compiled into each target.
- Remove the obsolete Qt resource file from the configuration foundation.

### Changed

- Version metadata now comes from `VERSION` and `app-profile.json`.
- MainWindow title, window size, Status Bar visibility and endpoint displays
  now use runtime configuration rather than hardcoded strings.
