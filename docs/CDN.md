# CDN layout

The development mock CDN follows the same product/channel/platform/architecture
path that production infrastructure will use:

```text
products/<slug>/<channel>/<platform>/<architecture>/
├── manifest.json
├── <artifact>
└── release-notes/<version>.json
```

For this template, the development manifest is:

```text
http://localhost:8088/products/myapp-template/development/linux/x64/manifest.json
```

`UpdateManager` reads only `manifest.json` in v0.1.4. The artifact and release
notes remain metadata until the download and verification feature is introduced.
Publish artifact and release notes before publishing the manifest so a manifest
never points to content that is unavailable yet.
