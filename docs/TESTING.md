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

## Manual smoke test for v0.1.2

1. Start the app normally.
2. Open **Settings → Appearance** and switch between System, Light and Dark.
3. Change Accent Color and Font Scale; confirm the preview is immediate.
4. Click Cancel before Apply; confirm the original appearance returns.
5. Change the Status Bar visibility in **Layout**; confirm the preview is
   immediate and persists after Apply and restart.
6. Toggle the Toolbar and confirm it persists after Apply and restart.
7. Change the Update Channel and CDN URL; Apply, restart and confirm the
   Template Dashboard and Status Bar reflect the values.
8. Use Reset Current Page and confirm the active profile values return.
9. Use Reset All Settings and confirm all local overrides are removed.
10. Start with `--profile staging` and confirm staging values remain active
    until a local override is set.
11. Run `./scripts/test.sh debug` and confirm all tests pass.
