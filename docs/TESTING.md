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

## Manual smoke test for v0.1.3

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
14. Run `./scripts/test.sh debug` and confirm all tests pass.
