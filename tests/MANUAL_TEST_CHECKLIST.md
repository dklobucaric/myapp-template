# Manual Test Checklist

## v0.1.1 Configuration Foundation

- [ ] App starts from any working directory.
- [ ] Template Dashboard shows dynamic app name, product ID and environment.
- [ ] App creates a runtime `config.json` on first start.
- [ ] Settings opens with all expected categories.
- [ ] Settings → Advanced displays the runtime config path.
- [ ] `--profile staging` displays staging endpoints and beta channel.
- [ ] User override in runtime `config.json` appears after restart.
- [ ] Malformed runtime config does not crash the application.
- [ ] Status Bar respects `layout.showStatusBar`.
- [ ] `./scripts/test.sh debug` passes.
