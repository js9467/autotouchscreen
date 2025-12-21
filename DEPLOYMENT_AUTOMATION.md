# Bronco Controls Deployment & OTA Playbook

This document describes how to prepare a shippable firmware bundle, give remote installers a single script that flashes a unit over USB, and keep devices updated over the air.

---

## 1. Build once, package once
1. Build the production firmware with PlatformIO: `pio run -e esp32s3box`.
2. Collect the artifacts from `.pio/build/esp32s3box/`:
   - `bootloader.bin`
   - `partitions.bin`
   - `boot_app0.bin`
   - `firmware.bin` (a copy of `firmware.bin` / `app.bin` renamed for clarity)
3. Copy the files into `dist/<version>/`. Example: `dist/1.4.0/firmware.bin`.
4. Mirror the same folder into `ota_functions/releases/<version>/`. The `releases` directory is `.gitignore`d, so you can drop private binaries there before running `flyctl deploy`.

> The flashing scripts expect those exact filenames. If the partition layout ever changes, update the offsets inside `tools/deploy/flash_device.ps1` and `.sh`.

---

## 2. Generate the OTA manifest
Use the helper script to calculate size and MD5 in one step:

```bash
node tools/deploy/build_manifest.mjs \
  --version 1.4.0 \
  --firmware dist/1.4.0/firmware.bin \
  --notes "Stable release"
```

Outputs `dist/1.4.0/manifest.json`:

```json
{
  "version": "1.4.0",
  "channel": "stable",
  "released_at": "2025-12-21T12:00:00Z",
  "notes": "Release notes go here",
  "firmware": {
    "url": "firmware.bin",
    "size": 3145728,
    "md5": "c0ffee..."
  }
}
```

When deploying to Fly, leave `--base-url` unset so the manifest references `firmware.bin`; the cloud service rewrites it to `https://image-optimizer-still-flower-1282.fly.dev/ota/releases/<version>/firmware.bin` automatically. If you prefer S3 or another host, pass `--base-url <https://...>` so the manifest carries a fully-qualified link.

---

## 3. Configure devices for OTA
The runtime now reads an `ota` block from `config.json`:

```json
"ota": {
  "enabled": true,
  "auto_apply": true,
  "manifest_url": "https://image-optimizer-still-flower-1282.fly.dev/ota/manifest",
  "channel": "stable",
  "check_interval_minutes": 60
}
```

- `manifest_url` should target the Fly service (`/ota/manifest`) unless you host releases elsewhere. Append `?channel=beta` to lock a device to a specific lane.
- `channel` is compared with the manifest `channel` to avoid flashing beta images onto production devices.
- `check_interval_minutes` is clamped between 5 minutes and 24 hours (default 60).
- When `auto_apply` is `false`, the firmware only logs that an update is available. (Hook `OTAUpdateManager::triggerImmediateCheck()` to a UI button later if needed.)

### OTA flow on device
1. Device enables Wi-Fi station mode once credentials are saved.
2. `OTAUpdateManager` waits for a successful STA connection, downloads the manifest, compares semantic versions (`x.y.z`).
3. If a newer build is available and `auto_apply = true`, it streams the binary using `HTTPClient` + `Update`, verifies the MD5, and reboots into the new slot.
4. After a successful update the stored `config.version` is updated so UI/API calls report the running version immediately.

---

## 4. Publish to the Fly OTA service
1. Copy your freshly built `dist/<version>/` folder into `ota_functions/releases/<version>/` (already done in step 1).
2. Deploy the Fly app from the repo root:
  ```bash
  flyctl deploy
  ```
  The Docker build bakes whatever lives under `ota_functions/releases` into `/app/releases` inside the container.
3. Verify everything is live:
  ```bash
  curl https://image-optimizer-still-flower-1282.fly.dev/ota/releases
  curl https://image-optimizer-still-flower-1282.fly.dev/ota/manifest
  ```
  You should see your new version listed along with the fully-qualified manifest/firmware URLs.

Each release folder is ignored by git, so you can keep private binaries locally, deploy to Fly, and remove them afterward.

---

## 5. Ship a “one-click” installer
Bundle a small ZIP for end users that only contains the four `.bin` files plus both flashing scripts.

### Windows (PowerShell)
- Script: `tools/deploy/flash_device.ps1`
- Capabilities:
  - Enumerates COM ports (`Get-CimInstance Win32_SerialPort`) and auto-selects CP210/USB-JTAG devices.
  - Downloads a portable `esptool.exe` release from Espressif (cached under `%LOCALAPPDATA%/BroncoControls`).
  - Flashes bootloader, partitions, boot_app0, and firmware in one command: `0x0`, `0x8000`, `0xe000`, `0x10000`.
- Usage for customers: right-click → “Run with PowerShell” or execute:
  ```powershell
  powershell -ExecutionPolicy Bypass -File flash_device.ps1 -PackagePath .
  ```
  Optional flags: `-Port COM5`, `-ListPorts`.

### macOS / Linux
- Script: `tools/deploy/flash_device.sh`
- Requirements: `python3 -m pip install esptool` (or supply `ESPTOOL` env var pointing at a binary).
- Usage:
  ```bash
  chmod +x flash_device.sh
  ./flash_device.sh -d ./dist/1.4.0
  ```
  Optional flags: `-p /dev/ttyACM0`, `-l` to list detected serial devices.

Both scripts print friendly progress and stop with a non-zero exit on failure so support staff immediately see what went wrong.

---

## 6. Provisioning experience for recipients
1. Recipient connects the unit with USB and runs the script (no PlatformIO or drivers required).
2. Firmware boots into AP+STA mode, showing `CAN-Control` SSID plus captive portal.
3. They join the AP, provide Wi-Fi credentials via the built-in web UI, and the device switches to their network.
4. Once online, the OTA manager immediately checks the manifest, installs the latest build if necessary, and reboots into production mode. Future checks run every `check_interval_minutes`.

The outcome: shipping teams only need to send the prepared ZIP + short instructions, and devices self-update afterward.

---

## 7. Hosting and monitoring tips
- Keep at least one previous firmware artifact accessible so you can roll users back by updating the manifest to the earlier version.
- Serve manifests and binaries over HTTPS when possible; the client accepts HTTP for lab setups but logs that TLS is disabled.
- Version naming should follow `major.minor.patch` so semantic comparison behaves as expected.
- For staged rollouts, publish multiple manifests (`stable`, `beta`) and point specific units at different `manifest_url` values.
- Instrument your hosting stack (Fly.io logs, S3 metrics) to confirm downloads succeed—each device checks roughly once an hour by default.

---

## 8. Future enhancements
- Add a UI “Check for updates” button that calls `OTAUpdateManager::triggerImmediateCheck()`.
- Extend the manifest format with release notes or minimum required config schema versions so the device can warn operators before applying incompatible builds.
- Digitally sign manifests and embed a root certificate in firmware for end-to-end authenticity.

With these pieces in place you can drop-ship units, let customers flash them with a single file, and be confident that every device converges to the latest software automatically.
