# Roadmap

## Completed

### v0.1.0 — Linux application shell foundation

- Public MIT-licensed repository
- Mock license-server and CDN contracts
- Menu Bar, Main Content Area and Status Bar
- Template Dashboard

### v0.1.1 — Configuration foundation

- Layered JSON configuration
- Qt resource-bundled product and environment profiles
- Runtime AppData `config.json`
- ConfigManager unit tests
- Settings Dialog shell

### v0.1.2 — Functional Settings

- Functional Settings pages with Apply, OK, Cancel and reset actions
- Live theme, accent color, font-scale and Status Bar preview
- Persistent local overrides for layout, update, CDN, license endpoint and
  diagnostics preferences
- ThemeManager application appearance subsystem
- Additional configuration reset and preview tests

### v0.1.3 — Logging and safe diagnostics

- Local structured logger with level filtering, redaction and rotation
- Diagnostics Settings actions for Logs Folder and Safe Support Package
- Local standard ZIP export with redacted runtime/config/log data
- Automated logging and diagnostics regression tests

### v0.1.4 — Update foundation

- `UpdateManager` asynchronous mock CDN manifest fetch
- Semantic-version comparison and manifest validation
- Manual and interval-aware automatic checks
- Redacted persisted `update-state.json`
- Dashboard and Status Bar update result display
- No artifact download or installation yet

### v0.1.5 — License foundation

- `LicenseManager` mock license-status checks
- Server-authoritative entitlement, duration and device-limit display
- Manual and recurring automatic license checks
- Redacted persisted `license-state.json`
- Offline grace and safe clock-anomaly behavior
- LicenseManager automated tests

## Planned

### v0.1.6 — Artifact download and verification

- Download artifact to a local cache
- SHA-256 content verification
- Clear download/cache failure states
- No installation or restart yet

### v0.1.7 — Release and tooling polish

- Release scripts
- Manifest generation validation
- Packaging workflow polish
- Branding/icon metadata for Linux, Windows and macOS
