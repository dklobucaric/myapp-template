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
