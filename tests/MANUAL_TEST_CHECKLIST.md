# Manual Test Checklist

## v0.1.5 License foundation, update foundation, logging and safe diagnostics

- [ ] App starts from any working directory.
- [ ] Template Dashboard shows dynamic app name, product ID and environment.
- [ ] App creates a runtime `config.json` on first start.
- [ ] Settings opens with all expected categories.
- [ ] Appearance: System, Light and Dark preview correctly.
- [ ] Accent color preview works and Cancel restores the previous appearance.
- [ ] Font scale preview works and persists after Apply/restart.
- [ ] Layout: Status Bar visibility previews and persists after Apply/restart.
- [ ] Updates & CDN: channel, interval and CDN URL persist after Apply/restart.
- [ ] License Server URLs persist after Apply/restart.
- [ ] Reset Current Page restores profile defaults for that page.
- [ ] Reset All Settings removes all local overrides after confirmation.
- [ ] Settings → General and Advanced open the runtime config folder.
- [ ] `--profile staging` displays staging endpoints and beta channel before local overrides.
- [ ] User override in runtime `config.json` appears after restart.
- [ ] Malformed runtime config does not crash the application.
- [ ] `./scripts/test.sh debug` passes.

- [ ] Diagnostics: With logging enabled, app startup creates `logs/app.log`.
- [ ] Diagnostics: Set log level to `error`; normal info activity is not appended.
- [ ] Diagnostics: Change log level from `error` to `info`, Apply, reopen Settings and restart; it remains `info`.
- [ ] Diagnostics: Disable logging, restart and confirm no new log lines are written.
- [ ] Diagnostics: Open Logs Folder opens the AppData `logs` directory.
- [ ] Diagnostics: Create Safe Support Package creates a local ZIP and reports its path.
- [ ] Diagnostics: ZIP contains README, runtime summary, config summary and log excerpt only.
- [ ] Diagnostics: ZIP does not contain config.json, license files, a known token/password/serial
      test value or the full home-directory path.


- [ ] Start `./scripts/run-dev-services.sh`; use **Help → Check for Updates**.
- [ ] Development manifest reports the mock 0.1.5 update and refreshes Dashboard and Status Bar.
- [ ] Check result does not download or install an artifact.
- [ ] Stop mock services and confirm the next manual check reports a safe network failure.
- [ ] Set automatic update checks to five minutes and confirm a background check occurs without a manual action.
- [ ] `update-state.json` has a timestamp and redacted manifest URL/message; it has no token query value.


- [ ] Start `./scripts/run-dev-services.sh`; use **Help → Check License Status**.
- [ ] Mock license reports Active and refreshes Dashboard and Status Bar.
- [ ] Manual license dialog shows a redacted serial suffix, annual 365-day duration and `1 / 3` devices.
- [ ] License Server Settings: automatic check preference and fallback interval persist after Apply/restart.
- [ ] Stop mock services; the next manual license check shows Offline grace when a valid cached response exists.
- [ ] A stopped server never crashes or locally locks the application.
- [ ] `license-state.json` contains no full serial or URL query token.
