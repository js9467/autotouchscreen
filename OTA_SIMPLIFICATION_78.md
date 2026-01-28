# OTA Simplification Changes - Version 1.3.78

## Summary
Converting from automatic OTA updates via Fly.io to manual-only updates using GitHub releases.

## Completed Changes

### 1. Version Update
- ✅ Updated `.version_state.json` from build 80 to build 78
- New version: **1.3.78**

### 2. OTA Manager Simplification  
- ✅ **src/ota_manager.h** - Removed automatic polling variables:
  - Removed `auto_apply_`
  - Removed `base_check_interval_ms_`
  - Removed `last_check_ms_`
  - Removed `internet_available_`
  - Removed failure tracking variables

- ✅ **src/ota_manager.cpp** - Simplified to manual-only:
  - `begin()` - Removed automatic check interval logic
  - `loop()` - Removed automatic polling, only processes manual requests
  - `applyManifest()` - Always requires manual confirmation (force_install flag)

## Remaining Manual Edits Needed

### 3. Config Files (File encoding issues prevented automated changes)

**src/config_types.h** - Line ~95, comment out:
```cpp
struct OTAConfig {
    bool enabled = true;
    // bool auto_apply = false;  // COMMENT THIS OUT - manual-only
    std::string manifest_url = kOtaManifestUrl;
    std::string channel = "stable";
    // std::uint32_t check_interval_minutes = 60;  // COMMENT THIS OUT - manual-only
};
```

**src/config_manager.cpp** - Three locations to update:

1. Line ~259 (default config):
```cpp
    cfg.ota.enabled = true;
    // cfg.ota.auto_apply = true;  // COMMENT OUT
    cfg.ota.manifest_url = kOtaManifestUrl;
    cfg.ota.channel = "stable";
    // cfg.ota.check_interval_minutes = 60;  // COMMENT OUT
```

2. Line ~413 (JSON serialization):
```cpp
    ota["enabled"] = source.ota.enabled;
    // ota["auto_apply"] = source.ota.auto_apply;  // COMMENT OUT
    ota["manifest_url"] = source.ota.manifest_url.c_str();
    ota["channel"] = source.ota.channel.c_str();
    // ota["check_interval_minutes"] = source.ota.check_interval_minutes;  // COMMENT OUT
```

3. Line ~596 (JSON deserialization):
```cpp
    if (!ota.isNull()) {
        target.ota.enabled = ota["enabled"] | target.ota.enabled;
        // target.ota.auto_apply = ota["auto_apply"] | target.ota.auto_apply;  // COMMENT OUT
        target.ota.channel = safeString(ota["channel"], target.ota.channel);
        // const std::uint32_t interval = ota["check_interval_minutes"] | target.ota.check_interval_minutes;  // COMMENT OUT
        // target.ota.check_interval_minutes = clampValue<std::uint32_t>(interval, 5u, 1440u);  // COMMENT OUT
    }
```

### 4. UI Updates (Not Yet Started)

**Device Settings Screen** (src/ui_builder.cpp):
- Change button from "Check for Updates" to "Select Firmware"
- Add firmware version dropdown/selector
- Change workflow: select version → download & install

**Web Interface** (src/web_interface.h):
- Remove automatic "Check Updates" button
- Add firmware version selector dropdown
- Add "Install Selected Firmware" button
- Update JavaScript functions `checkForUpdates()` and `triggerOTAUpdate()`

### 5. Backend API Updates (Not Yet Started)

**src/web_server.cpp**:
- Keep `/api/ota/check` but simplify to just list available versions
- Update `/api/ota/update` to accept version parameter
- Consider adding `/api/ota/versions` endpoint for listing available firmware

### 6. Remove Fly.io, Switch to GitHub (Not Yet Started)

**Manifest URL Update**:
- Change from: `https://bronco-controls-ota.fly.dev/manifest.json`
- Change to: GitHub releases API or direct manifest hosting
- Example: `https://raw.githubusercontent.com/YOUR_REPO/main/firmware/manifest.json`

**Files to Remove/Archive**:
- `ota_functions/` directory (entire Fly.io server)
- `fly.toml` (root)
- `ota_functions/fly.toml`
- `deploy.ps1` - Remove Fly deployment steps
- `deploy_169.ps1` - Remove Fly deployment

**Files to Update**:
- `src/config_manager.cpp` line ~126 - Update default manifest URL
- `src/config_types.h` - Update `kOtaManifestUrl` constant

## Simplified Workflow (Target)

### Current (Auto):
1. Device polls Fly.io every hour
2. Auto-downloads if newer version available  
3. User clicks "Update" to install

### New (Manual):
1. User opens Settings
2. User clicks "Check Firmware" or sees version dropdown
3. User selects desired version from list
4. User clicks "Install" 
5. Firmware downloads and installs

## GitHub Releases Approach

**Option 1: GitHub Releases API**
- Store firmware.bin files as release assets
- Query: `https://api.github.com/repos/OWNER/REPO/releases`
- Parse JSON for version list and download URLs

**Option 2: Static Manifest**
- Host `manifest.json` in repository
- List all available versions and URLs
- Simpler, no API rate limits

**Recommended manifest.json format**:
```json
{
  "versions": [
    {
      "version": "1.3.78",
      "channel": "stable",
      "released_at": "2026-01-27",
      "firmware_url": "https://github.com/USER/REPO/releases/download/v1.3.78/firmware.bin",
      "md5": "abc123...",
      "size": 1234567
    },
    {
      "version": "1.3.77",
      "channel": "stable",
      "released_at": "2026-01-20",
      "firmware_url": "https://github.com/USER/REPO/releases/download/v1.3.77/firmware.bin",
      "md5": "def456...",
      "size": 1234000
    }
  ]
}
```

## Testing Checklist

After completing all changes:
- [ ] Compile firmware successfully
- [ ] Flash to device
- [ ] Verify Settings screen shows firmware selector
- [ ] Verify web interface shows firmware selector
- [ ] Test manual firmware selection and download
- [ ] Verify no automatic polling occurs
- [ ] Test with GitHub-hosted manifest
- [ ] Remove all Fly.io dependencies

## Next Steps

1. **Manual Config Edits** - Complete the 4 config file changes above
2. **Build Test** - Compile to check for errors
3. **UI Updates** - Modify device and web settings screens
4. **Backend API** - Update OTA endpoints
5. **GitHub Setup** - Create manifest and host firmware
6. **Deploy Script** - Update to use GitHub instead of Fly.io
7. **Test** - End-to-end firmware update test
