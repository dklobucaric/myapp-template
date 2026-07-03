# Terminology

## Menu Bar

The top-level application navigation containing menus such as File, Edit, Settings and Help.

## Menu

A category inside the Menu Bar, such as File or Help.

## Action

A clickable command inside a menu.

## Main Content Area

The central visible area of the application.

Preferred internal name: `Workspace`.

## Workspace

The product-specific content hosted in the Main Content Area.

MyAppTemplate initially uses a Template Dashboard workspace.

## Status Bar

The bottom application bar used for short runtime status information.

Examples:

- Ready
- License: Active
- Updates: Up to date
- Update available

## Update Indicator

A clickable status element in the Status Bar showing update state.

## License Indicator

A clickable status element in the Status Bar showing license state.

## Settings Dialog

The main settings window with navigation categories and Settings Pages.

## Settings Page

One configuration category inside the Settings Dialog.

Examples:

- General
- Appearance
- Layout
- Updates & CDN
- License Server
- Diagnostics
- Advanced

## Product Profile

The application identity and default settings stored in `app-profile.json`.

## Environment Profile

A development, staging or production configuration overlay.

## Update Channel

The release channel used for updates.

Supported channels:

- development
- beta
- stable

## License Server

The backend service responsible for users, products, licenses, activations and license API responses.

## CDN

The content delivery location used for installers, release notes and update manifests.

## Diagnostics Bundle

A local support ZIP containing safe runtime information, a redacted config summary
and a bounded redacted log excerpt. The app never uploads it automatically.

## ConfigManager

Reusable core service that loads and merges the app profile, default config,
environment profile and local user config into one `AppConfig` runtime model.

## AppConfig

The normalized runtime configuration model consumed by the application shell.

## AppLogger

The local-only structured logger. It applies the Diagnostics logging level,
redacts common secret-shaped values and private runtime paths, and rotates logs.

## DiagnosticsManager

The service that creates a Safe Support Package locally without copying config,
license or credential files.


## LicenseManager

The reusable client that checks the server-authoritative license status,
persists a redacted local state and schedules online verification.

## License Identifier / Serial

An opaque identifier for an entitlement. It does not encode the product,
duration or number of devices.

## Device Limit

The server-enforced maximum number of active installations for a license.

## Offline Grace

A temporary cached state used after a previously verified license cannot reach
the server. It is bounded by the server-provided grace timestamp.
