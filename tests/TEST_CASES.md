# Test Cases

## Configuration foundation

| ID | Scenario | Expected result |
| --- | --- | --- |
| CFG-001 | Start with no user config file | App creates a minimal user `config.json` atomically. |
| CFG-002 | Load development profile | Local mock CDN and license URLs are used. |
| CFG-003 | Load staging profile | Staging endpoints and beta update channel are used. |
| CFG-004 | User override changes CDN URL | User value overrides the environment profile value. |
| CFG-005 | User config contains malformed JSON | App stays usable, warns and falls back safely. |
| CFG-006 | Open Settings | Every expected settings category is visible. |
