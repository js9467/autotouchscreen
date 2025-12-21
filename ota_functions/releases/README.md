# OTA Release Drop Folder

Place unsigned release payloads here **before** running `flyctl deploy`.

Structure per version:

```
ota_functions/releases/
  1.4.0/
    firmware.bin        # Built via PlatformIO (copy of app binary)
    manifest.json       # Generated with tools/deploy/build_manifest.mjs
```

Guidelines:
- Keep firmware filenames consistent with the manifest (`firmware.url` can stay `firmware.bin`).
- Versions are discovered from the folder name; stick to `major.minor.patch` so sorting works.
- Files in this directory are ignored by git, so you can keep private binaries without polluting history.
- After copying files, deploy with `flyctl deploy` to bake them into the Fly image.

At runtime the Fly service serves the latest manifest at `/ota/manifest` and exposes per-version assets under `/ota/releases/<version>/`.
