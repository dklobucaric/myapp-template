# Test Cases

## Configuration foundation

| ID | Scenario | Expected result |
| --- | --- | --- |
| CFG-001 | Start with no user config file | App creates a minimal user `config.json` atomically. |
| CFG-002 | Load development profile | Local mock CDN and license URLs are used. |
| CFG-003 | Load staging profile | Staging endpoints and beta update channel are used. |
| CFG-004 | User override changes CDN URL | User value overrides the environment profile value. |
| CFG-005 | User config contains malformed JSON | App stays usable, warns and falls back safely. |
| CFG-006 | Clear user overrides | Environment profile values become effective again. |

## Functional Settings

| ID | Scenario | Expected result |
| --- | --- | --- |
| SET-001 | Change theme and cancel | Preview reverts to the last applied appearance. |
| SET-002 | Change accent color and Apply | Accent updates immediately and persists after restart. |
| SET-003 | Change font scale and Apply | Font scale updates immediately and persists after restart. |
| SET-004 | Hide Status Bar and Apply | Status Bar hides immediately and remains hidden after restart. |
| SET-005 | Change channel and CDN URL | Dashboard and Status Bar display the persisted update settings. |
| SET-006 | Reset Current Page | Only the selected page's local overrides are removed. |
| SET-007 | Reset All Settings | All local overrides are removed after confirmation. |

## Logging and safe diagnostics

| ID | Scenario | Expected result |
| --- | --- | --- |
| LOG-001 | Logging disabled | No log file is created or appended. |
| LOG-002 | Log level set to warning | Debug and info messages are filtered; warning/error messages remain. |
| LOG-003 | Secret-shaped value in a message | Password, token, serial and bearer values are redacted before disk write. |
| LOG-004 | Log reaches rotation limit | Current log rotates and only configured archive count remains. |
| DIA-001 | Create safe support package | Local standard ZIP is created; no upload occurs. |
| DIA-002 | Inspect package content | It includes runtime/config summaries and a bounded log excerpt, not config.json or licenses. |
| DIA-003 | Sensitive/path regression | ZIP has no known secrets, token query values or full home path. |


## Update foundation

| ID | Scenario | Expected result |
| --- | --- | --- |
| UPD-001 | Valid newer manifest | Update available with current and available version. |
| UPD-002 | Manifest version equals current version | Up-to-date state is reported. |
| UPD-003 | Invalid or missing manifest fields | Safe invalid-manifest result; no crash. |
| UPD-004 | Platform/architecture mismatch | Unsupported-platform result. |
| UPD-005 | CDN unavailable | Network error result is persisted without query secrets. |
| UPD-006 | Manual check | Dashboard and Status Bar refresh; no artifact is downloaded. |


## License foundation

| ID | Scenario | Expected result |
| --- | --- | --- |
| LIC-001 | Valid active response | Active server status, redacted serial, duration and device count are displayed. |
| LIC-002 | Product ID mismatch or malformed response | Safe invalid-response result; no crash. |
| LIC-003 | Server reports expired or revoked | Server status is preserved and shown without local inference. |
| LIC-004 | License server unavailable after active verification | Offline grace is shown while the cached server grace timestamp is valid. |
| LIC-005 | Local clock appears earlier than cached server time | Client warns/retries later; it does not locally lock the app. |
| LIC-006 | Cached grace may have elapsed while offline | Online verification required; client does not locally declare revocation. |
| LIC-007 | Automatic license checks enabled | Due checks run in the background; settings changes reschedule them. |
| LIC-008 | Inspect `license-state.json` | No full serial, token, password or query secret is persisted. |
