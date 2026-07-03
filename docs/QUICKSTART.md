# Quick Start

## Requirements

- C++20 compiler
- CMake 3.24 or newer
- Ninja
- Qt 6 Core, Widgets and Test development packages

## Build

```bash
./scripts/build.sh debug
./build/linux-debug/myapp-template
```

## Run automated tests

```bash
./scripts/test.sh debug
```

## Run with a selected environment profile

```bash
./build/linux-debug/myapp-template --profile development
./build/linux-debug/myapp-template --profile staging
./build/linux-debug/myapp-template --profile production
```

## Where to begin a new application

- Replace or extend `src/workspace/TemplateDashboard.cpp` for the new Main Content Area.
- Update `assets/defaults/app-profile.json` with the new product identity.
- Update the relevant profile endpoints under `profiles/`.
