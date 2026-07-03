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

- initial user `config.json` creation
- environment profile endpoint overrides
- user config override precedence
- safe fallback after malformed user JSON

## Manual smoke test for v0.1.1

1. Start the app normally.
2. Confirm the Template Dashboard shows values from JSON configuration.
3. Open Settings and move through every category.
4. Check Settings → Advanced for the runtime config path.
5. Start with `--profile staging` and confirm the staging endpoint appears in
   the dashboard and Settings.
6. Edit the local `config.json` manually, restart, and confirm the override is
   visible.
7. Put malformed JSON in local `config.json`, start again, and confirm the app
   stays open and shows a configuration warning.
