# Mock license server

This static development fixture is served by:

```bash
./scripts/run-dev-services.sh
```

Endpoint:

```text
http://localhost:8089/api/v1/license-status.json
```

It returns one active annual entitlement with a three-device limit. Edit the
fixture to test `active`, `grace`, `expired` and `revoked` response states.
The mock is intentionally static; it does not perform activation, ordering or
payment processing.
