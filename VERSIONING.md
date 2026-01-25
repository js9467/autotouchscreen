# Versioning System

## Overview

The versioning system is designed to prevent loops and ensure consistent version tracking across builds and OTA updates.

## How It Works

### Single Source of Truth: `.version_state.json`

```json
{
  "major": 1,
  "minor": 3,
  "build": 66
}
```

This file contains the current version and is the ONLY file you should manually edit for version changes.

### Build Process

1. **PlatformIO Build** triggers `tools/versioning.py` (configured in `platformio.ini`)
2. **versioning.py** reads `.version_state.json`
3. **Generates** `src/version_auto.h` with the version string
4. **Firmware** is compiled with this version embedded

### Deployment Process

Use `deploy.ps1` for all deployments:

```powershell
# Patch version (1.3.66 -> 1.3.67)
.\deploy.ps1 -VersionBump patch

# Minor version (1.3.66 -> 1.4.0)
.\deploy.ps1 -VersionBump minor

# Major version (1.3.66 -> 2.0.0)
.\deploy.ps1 -VersionBump major
```

The script:
1. Reads current version from `.version_state.json`
2. Increments the version
3. Updates `.version_state.json` (source of truth)
4. Updates `src/version_auto.h` (for consistency)
5. Builds firmware with new version
6. Creates OTA release package
7. Commits and pushes to git
8. Deploys to Fly.io

## OTA Update Flow

1. Device compares `APP_VERSION` (from firmware) with remote manifest version
2. If remote is newer, offers/applies update
3. After update, device boots with new firmware
4. `main.cpp` auto-detects version change and updates `config.version`

## Why This Prevents Loops

**The Problem (Before Fix):**
- `.version_state.json` stuck at 66
- Builds kept using 66 even when trying to update to 67
- Device downloaded "new" firmware but it was still 66
- Loop: check → download → reboot → still 66 → check again...

**The Solution (After Fix):**
- `.version_state.json` is the single source of truth
- `deploy.ps1` increments it BEFORE building
- Firmware gets new version baked in
- OTA comparison works correctly
- No more loops!

## Manual Version Changes

If you need to manually set a version:

1. Edit `.version_state.json`:
   ```json
   {
     "major": 1,
     "minor": 5,
     "build": 0
   }
   ```

2. Run a build to regenerate `version_auto.h`:
   ```powershell
   pio run -e waveshare_7in
   ```

3. Deploy if needed:
   ```powershell
   .\deploy.ps1 -VersionBump patch
   ```

## Files in the System

| File | Purpose | Edit? |
|------|---------|-------|
| `.version_state.json` | Single source of truth for version | ✅ Manually or via deploy.ps1 |
| `src/version_auto.h` | Generated header file | ❌ Auto-generated |
| `tools/versioning.py` | Reads state, generates header | ❌ Don't modify |
| `deploy.ps1` | Deployment script | ⚠️ Only for workflow changes |

## Troubleshooting

### "Version stuck, not incrementing"

1. Check `.version_state.json` - is it the version you expect?
2. Use `deploy.ps1` to bump version (don't edit files directly)
3. Verify build output shows new version

### "OTA update loop"

1. Check device version: Settings → About
2. Check manifest: https://image-optimizer-still-flower-1282.fly.dev/ota/manifest
3. Ensure manifest version > device version
4. If stuck, deploy a new version with `deploy.ps1`

### "Git conflicts on version files"

Always pull before deploying. If conflicts occur:
1. Accept incoming `.version_state.json`
2. Discard changes to `src/version_auto.h` (it's auto-generated)
3. Run build to regenerate header

## Verification & Safety

Before deploying, always run the verification script:

```powershell
.\verify_ota_package.ps1 -Version "1.3.68"
```

This checks:
1. ✓ `.version_state.json` has correct version
2. ✓ `version_auto.h` matches state file
3. ✓ Firmware binary exists
4. ✓ OTA package directory is complete
5. ✓ All manifests are consistent with correct MD5

The `deploy.ps1` script now runs this automatically before deploying to Fly.io.

## Best Practices

✅ **DO:**
- Use `deploy.ps1` for all deployments
- Commit `.version_state.json` to git
- Let the system auto-generate `version_auto.h`
- Run verification before manual deployments
- Check GitHub Actions for version consistency warnings

❌ **DON'T:**
- Manually edit `src/version_auto.h`
- Modify `tools/versioning.py` without understanding the system
- Deploy without incrementing version
- Skip git commits when deploying
- Deploy without running verification first

## Summary

**Old way (broken):**
- Manual edits to `version_auto.h`
- Inconsistent versioning
- OTA loops

**New way (fixed):**
- `.version_state.json` = single source of truth
- `deploy.ps1` = single deployment method
- Automatic, consistent, loop-free versioning
