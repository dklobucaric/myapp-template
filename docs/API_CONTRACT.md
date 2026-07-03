# API contracts

## Update manifest v1

`UpdateManager` requests a JSON manifest from the configured CDN path. Required
fields are:

```json
{
  "schemaVersion": 1,
  "productId": "hr.ddlab.myapp-template",
  "channel": "development",
  "platform": "linux",
  "architecture": "x64",
  "version": "0.1.5",
  "publishedAt": "2026-07-03T20:00:00Z",
  "artifactFileName": "MyAppTemplate-0.1.5.AppImage",
  "artifactUrl": "https://cdn.example.invalid/path/MyAppTemplate-0.1.5.AppImage",
  "sha256": "<64 lowercase-or-uppercase hexadecimal characters>",
  "sizeBytes": 0,
  "releaseNotesUrl": "https://cdn.example.invalid/path/release-notes/0.1.5.json",
  "mandatory": false
}
```

The v0.1.4 client validates this contract but does not download `artifactUrl`.
A future download feature will verify the content digest before presenting an
installer/restart step.


## License status response v1

`LicenseManager` requests `license-status.json` below the configured License API
base URL. The server returns the current entitlement, not a locally calculated
license result.

```json
{
  "schemaVersion": 1,
  "success": true,
  "data": {
    "licenseId": "lic_example_000001",
    "serial": "DDLA-7J4K-P9Q2-M8X5",
    "productId": "hr.ddlab.myapp-template",
    "status": "active",
    "planName": "Developer Annual",
    "durationDays": 365,
    "remainingDays": 365,
    "activeDeviceCount": 1,
    "deviceLimit": 3,
    "features": ["updates", "diagnostics"],
    "issuedAt": "2026-07-03T16:00:00Z",
    "serverTime": "2026-07-03T16:00:00Z",
    "expiresAt": "2027-07-03T23:59:59Z",
    "graceUntil": "2027-07-17T23:59:59Z",
    "nextOnlineCheckAfter": "2026-07-03T17:00:00Z"
  }
}
```

`serial` identifies an entitlement; it must not encode duration, product or
seat count. The backend is responsible for activation enforcement. The client
stores a redacted serial suffix only in `license-state.json`.
