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
| SET-005 | Hide Toolbar and Apply | Toolbar hides immediately and remains hidden after restart. |
| SET-006 | Change channel and CDN URL | Dashboard and Status Bar display the persisted update settings. |
| SET-007 | Reset Current Page | Only the selected page's local overrides are removed. |
| SET-008 | Reset All Settings | All local overrides are removed after confirmation. |
