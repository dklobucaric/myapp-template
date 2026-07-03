# Testing

## Linux build and test

```bash
./scripts/build.sh debug
./scripts/test.sh debug
```

The test runner configures the selected CMake preset, builds the app and runs
CTest.

## Current automated tests

`ConfigManagerTests` verifies:

- embedded configuration resources are available
- initial user `config.json` creation
- environment profile endpoint overrides
- user config override precedence
- safe fallback after malformed user JSON
- clearing user overrides restores environment values
- preview configuration uses the baseline correctly when overrides change
- logging-level filtering, redaction and rotation
- safe diagnostics ZIP creation without secret values or private home paths
- update manifest success, invalid-data, compatibility, semantic-version and network-failure cases

## Manual smoke test for v0.1.4

1. Start the app normally.
2. Open **Settings → Appearance** and switch between System, Light and Dark.
3. Change Accent Color and Font Scale; confirm the preview is immediate.
4. Click Cancel before Apply; confirm the original appearance returns.
5. Change the Status Bar visibility in **Layout**; confirm the preview is
   immediate and persists after Apply and restart.
6. Change the Update Channel and CDN URL; Apply, restart and confirm the
   Template Dashboard and Status Bar reflect the values.
7. Use Reset Current Page and confirm the active profile values return.
8. Use Reset All Settings and confirm all local overrides are removed.
9. Start with `--profile staging` and confirm staging values remain active
   until a local override is set.
10. Open **Settings → Diagnostics** and confirm logs are created only when logging is enabled.
11. Use **Open Logs Folder** and confirm the local `logs` folder opens.
12. Use **Create Safe Support Package**, inspect the generated ZIP and confirm it contains
    `README.txt`, `runtime-summary.txt`, `config-summary.json` and `log-excerpt.txt`.
13. Confirm the package was not uploaded anywhere and does not contain known credentials,
    serial values or a full home-directory path.
14. Start `./scripts/run-dev-services.sh`, then use **Help → Check for Updates**.
15. Confirm the development mock manifest reports an available version, updates the Dashboard
    and Status Bar, and does not download or install anything.
16. Stop mock services and check again; confirm a non-secret network error is shown and stored.
17. Set automatic checks to five minutes, leave the app open and confirm the next background
    check updates the Dashboard and Status Bar without using the manual menu action.
18. On **Settings → Diagnostics**, change the log level from `error` to `info`, click Apply,
    and confirm the selection remains `info` after reopening Settings and restarting the app.
17. Run `./scripts/test.sh debug` and confirm all tests pass.
