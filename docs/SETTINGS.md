# Settings

## Purpose

The Settings Dialog edits only the local user override layer in `config.json`.
It does not copy all values from the active environment profile into the user
file. This keeps a staging or production profile portable when a user has not
intentionally changed a setting.

## Actions

- **Apply** saves current values with atomic `QSaveFile` persistence and keeps
the dialog open.
- **OK** saves current values and closes the dialog.
- **Cancel** discards un-applied changes and restores any live appearance or
Status Bar preview to the most recently applied values.
- **Reset Current Page** removes local overrides relevant to the selected page.
- **Reset All Settings** removes every local override after confirmation.

## Pages

### General

Shows the active environment, application identity and runtime config location.
It also opens the local configuration folder.

### Appearance

- Theme: System, Light or Dark
- Accent color
- Font scale

Theme, accent color and font scale preview immediately. They persist after
Apply or OK and are restored when Cancel is used before applying changes.

### Layout

- Initial window width and height
- Show Status Bar

The Status Bar previews immediately. Initial dimensions are used on
the next application start.

### Updates & CDN

- Update channel
- Automatic update checks
- Check interval
- Automatic download preference
- CDN Base URL

This page saves preferences only. Connection checks arrive with UpdateManager.

### License Server

- License Portal URL
- License API URL
- Automatic license checks
- Fallback check interval

This page saves local endpoint and scheduling overrides. The server remains
authoritative for status, duration, expiry and device limits. After a successful
response, the server-provided next-check timestamp takes precedence over the
fallback interval.

### Diagnostics

- Logging enabled
- Log level
- Open Logs Folder
- Create Safe Support Package

Logging is local-only and changes take effect immediately after Apply or OK.
The support package is created locally as a ZIP; the application never uploads
it automatically. It contains safe summaries and a redacted bounded log excerpt,
not a copy of `config.json`, license data, credentials, tokens or full home paths.
See [`DIAGNOSTICS.md`](DIAGNOSTICS.md) for details.

### Advanced

Shows the active environment, config path and configuration precedence. It also
opens the config folder.

## Configuration precedence

```text
default config
→ app-profile defaults
→ environment profile
→ user config.json overrides
```

A reset removes only values in the final local override layer. The active
environment profile then becomes visible again.
