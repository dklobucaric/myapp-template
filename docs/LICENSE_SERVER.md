# License Server

## Purpose

`LicenseManager` is the desktop client for the configured license API. It checks
license status asynchronously and stores only a redacted local status cache in
`license-state.json`.

The server is authoritative for:

- license status (`active`, `grace`, `expired`, `revoked`);
- serial / license identifier;
- product entitlement;
- plan and purchased duration;
- expiry and grace dates;
- device / activation limits;
- the next preferred online verification time.

The desktop app does **not** infer a duration or seat count from a serial. A
serial is an opaque identifier, for example `DDLA-7J4K-P9Q2-M8X5`.

## Ordering and renewal model

The backend creates or updates an entitlement record when an order is completed.
For example:

- monthly order → `durationDays: 30` and an expiry 30 days after the service
  period begins;
- annual order → `durationDays: 365` and an expiry one year after the service
  period begins;
- a renewal normally extends `expiresAt` on the same license identifier;
- a server-side reissue can replace a serial without changing the client model.

The backend tracks device activations. `deviceLimit` is the maximum allowed
active installations; `activeDeviceCount` is informational to the desktop
client. Actual activation decisions remain server-side.

## Mock endpoint

Development uses the static endpoint:

```text
http://localhost:8089/api/v1/license-status.json
```

Start it together with the mock CDN:

```bash
./scripts/run-dev-services.sh
```

## Offline behavior

After a successful response, the client caches a redacted result. If the next
online check fails and the cached server grant is still within its
server-provided `graceUntil`, the UI shows **Offline grace**.

The local clock is never used by itself to lock the application. A clock moving
backwards logs a warning and triggers a later online retry. If the cached grace
window may have elapsed while offline, the UI says **Online verification
required** rather than locally declaring the license revoked or locking the
app.

## Deferred

v0.1.5 does not yet include license import, activation requests, signatures,
encryption, a user portal, payment processing or a production server.
