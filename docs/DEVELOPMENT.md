# Development

## Daily workflow

```bash
git switch main
git pull --ff-only
git switch -c feature/short-feature-name

./scripts/build.sh debug
./scripts/test.sh debug
```

Keep the working tree clean before packaging a hand-off ZIP or creating a
release tag.

## Source layout

```text
src/app/        Application bootstrap and command-line handling
src/core/       Reusable non-UI services such as ConfigManager
src/ui/         MainWindow and reusable application-shell UI
src/settings/   Settings Dialog and Settings Pages
src/workspace/  Product-specific Main Content Area
```

## Configuration development

Use an environment profile while testing endpoint changes:

```bash
./build/linux-debug/myapp-template --profile development
./build/linux-debug/myapp-template --profile staging
./build/linux-debug/myapp-template --profile production
```

Do not edit compiled resource paths in C++ for normal endpoint changes. Update
`app-profile.json`, `profiles/*.json`, or a local runtime `config.json`
override instead.

## Validation before commit

```bash
./scripts/build.sh debug
./scripts/test.sh debug
git diff --check
git status
```

## Qt Resources

Built-in JSON files are compiled into each target through `resources/myapp_template.qrc`. CMake `CMAKE_AUTORCC` compiles the resource collection into both the application and unit-test binaries, so `ConfigManager` can load `:/defaults/...` and `:/profiles/...` paths without manual resource initialization.
