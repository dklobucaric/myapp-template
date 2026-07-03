# Updates

## v0.1.4 scope

`UpdateManager` checks the active channel manifest only. It does not download,
install, replace, restart, or close the running application.

The manifest URL is derived from the configured CDN base URL, channel, platform
and architecture. Linux distribution names such as Linux Mint, Ubuntu and Fedora
are normalized to the shared `linux` artifact family:

```text
<cdnBaseUrl>/<channel>/<platform>/<architecture>/manifest.json
```

For the development profile this resolves to:

```text
http://localhost:8088/products/myapp-template/development/linux/x64/manifest.json
```

## Result states

- Not checked
- Checking
- Up to date
- Update available
- Network error
- Invalid manifest
- Unsupported platform or architecture
- Disabled by product profile

Manual checks are available through **Help → Check for Updates**. Automatic
checks run only when enabled. `UpdateManager` schedules the next check from the
persisted `update-state.json` timestamp, reschedules immediately when update
settings change, and schedules another check after every terminal result. A
backwards local clock triggers a new check rather than suppressing checks forever.

## Manifest contract

The v1 manifest requires:

- `schemaVersion`
- `productId`, `channel`, `platform`, `architecture`
- semantic `version`
- `publishedAt`
- `artifactFileName`, `artifactUrl`, `sha256`, `sizeBytes`
- `releaseNotesUrl`
- `mandatory`

The app validates product identity, current channel, current platform and CPU
architecture before it reports an update. URLs and error messages are passed
through the logger's redaction layer before persistence to logs or update state.

## Local state and privacy

The last terminal result is stored atomically in:

```text
<AppData>/update-state.json
```

It has a timestamp, result status, versions and redacted message/manifest URL.
No token-bearing query string is retained.

Artifact download plus SHA-256 verification belongs to the following release;
installation and restart remain a later, explicit user-approved phase.
