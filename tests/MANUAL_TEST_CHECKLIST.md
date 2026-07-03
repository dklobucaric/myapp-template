# Manual Test Checklist

## v0.1.2 Functional Settings

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
