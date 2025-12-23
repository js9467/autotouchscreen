# Automated Build & Deploy

This GitHub Actions workflow automatically builds, versions, and deploys firmware on every commit to `main`.

## What it does

1. **Auto-increments** patch version (e.g., 1.2.6 → 1.2.7)
2. **Builds** firmware using PlatformIO
3. **Creates** release directory with firmware.bin and manifest.json
4. **Commits** the built release back to the repo
5. **Deploys** to Fly.io OTA server automatically

## Setup

### 1. Get your Fly.io API token

```bash
flyctl auth token
```

### 2. Add to GitHub Secrets

1. Go to your repo: **Settings** → **Secrets and variables** → **Actions**
2. Click **New repository secret**
3. Name: `FLY_API_TOKEN`
4. Value: Paste the token from step 1
5. Click **Add secret**

### 3. Commit and push

That's it! Every commit to `main` will now:
- Auto-build firmware
- Auto-version (increments patch number)
- Auto-deploy to OTA server
- Make it available for OTA updates

## Workflow triggers

The workflow runs on:
- ✅ Push to `main` branch
- ❌ Skips if only these changed:
  - `ota_functions/releases/**` (to avoid recursion)
  - `.github/**` (workflow files)
  - `**.md` (documentation)

## Manual version bump

To bump minor or major version manually, edit [src/version_auto.h](../src/version_auto.h):

```cpp
constexpr const char* APP_VERSION = "1.3.0";  // Manual bump
```

Then commit. The workflow will continue from this version.

## Viewing build status

- **GitHub Actions tab** shows build progress
- **Check mark** on commit = successful build & deploy
- **Red X** = build failed (click for details)

## Disable auto-deploy

To push code without triggering a build, include `[skip ci]` in your commit message:

```bash
git commit -m "Update docs [skip ci]"
```

## Local testing

To test the workflow locally:

```bash
# Build firmware
pio run

# Run versioning script manually
tools/versioning.py

# Deploy to Fly.io
cd ota_functions
flyctl deploy --remote-only
```
