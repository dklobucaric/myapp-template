# Architecture

## Runtime configuration

`ConfigManager` is the first reusable core subsystem.

```text
Qt resources
├── app-profile.json
├── default config.json
└── environment profile
        ↓
User AppData config.json overrides
        ↓
AppConfig runtime model
        ↓
MainWindow
├── ThemeManager
├── AppLogger
├── Template Dashboard
├── Functional Settings Dialog
├── DiagnosticsManager
└── Status Bar
```

The application loads compiled Qt resources rather than relying on the current
working directory. This keeps the default configuration available after
packaging on Linux, Windows and macOS.

## Settings Dialog

The v0.1.2 Settings Dialog is functional. It writes only user overrides and
supports Apply, Cancel, Reset Current Page and Reset All Settings.

Categories:

- General
- Appearance
- Layout
- Updates & CDN
- License Server
- Diagnostics
- Advanced

`SettingsDialog` previews live appearance and visibility changes through a
callback to `MainWindow`. `ThemeManager` owns platform palette capture and
applies System, Light and Dark variants without making the system palette
unrecoverable.

## Built-in JSON Resources

Default JSON files and environment profiles are embedded into each executable
through `resources/myapp_template.qrc`. CMake `CMAKE_AUTORCC` is explicitly
enabled before targets are created; `qt_standard_project_setup()` enables
AUTOMOC/AUTOUIC but does not enable AUTORCC.

## Logging and safe diagnostics

`AppLogger` is a local-only structured logger. It applies the Diagnostics
settings immediately, filters messages by level, redacts secret-shaped values
and private runtime paths, and rotates `app.log` before it grows without bound.

`DiagnosticsManager` creates a standard ZIP locally. It contains only a
runtime summary, a safe configuration summary and a bounded, redacted excerpt
of the current log. It never copies `config.json`, licenses, passwords, tokens
or complete home-directory paths. The archive is not uploaded by the app.
