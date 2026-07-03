# Architecture

## Runtime configuration

`ConfigManager` is the first reusable core subsystem.

```text
Qt resources
├── app-profile.json
├── default config.json
└── environment profile
        ↓
User AppData config.json
        ↓
AppConfig runtime model
        ↓
MainWindow
├── Template Dashboard
├── Settings Dialog
└── Status Bar
```

The application loads compiled Qt resources rather than relying on the current
working directory. This keeps the default configuration available after
packaging on Linux, Windows and macOS.

## Settings Dialog

The v0.1.1 Settings Dialog is a read-only shell. It establishes the reusable
navigation structure before editable controls are added in later milestones.

Categories:

- General
- Appearance
- Layout
- Updates & CDN
- License Server
- Diagnostics
- Advanced


## Built-in JSON Resources

Default JSON files and environment profiles are embedded into each executable through `resources/myapp_template.qrc`. CMake `CMAKE_AUTORCC` is explicitly enabled before targets are created; `qt_standard_project_setup()` enables AUTOMOC/AUTOUIC but does not enable AUTORCC.
