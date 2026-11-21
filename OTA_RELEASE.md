# ESP32-S3 RGB Lamp OTA setup and releases

The sketch keeps the existing Wi-Fi setup/recovery flow and RGB controller. OTA uses an HTTPS JSON manifest, an HTTPS firmware image, the configured CA certificate, and a SHA-256 check before the new image is activated.

## Configure OTA once

1. In the sketch, change `FIRMWARE_VERSION` to the version being built.
2. Set `OTA_MANIFEST_URL` to the permanent HTTPS URL of `version.json`.
3. Replace the placeholder in `OTA_ROOT_CA` with the PEM root/intermediate CA certificate that validates both the manifest and firmware host. Certificate validation is enabled; the sketch does not disable TLS checks.
4. In Arduino IDE, select the exact ESP32-S3 board and a partition scheme that provides two app slots (OTA). Check the board vendor's flash size and scheme options; do not select a scheme based only on a guessed module name.
5. Compile and inspect `/ota/status` from the lamp controller. It reports whether the active partition table has a next OTA app slot.
6. If the currently installed partition table has no OTA slot, connect USB and upload this firmware once with the OTA-capable scheme selected. Future firmware images can then be installed from the controller's Firmware update section.

Changing the sketch alone cannot add an OTA app slot to a device that was flashed with a non-OTA partition table.

## Build and publish a release

1. Increase `FIRMWARE_VERSION` before each release, for example `1.0.1`.
2. In Arduino IDE, choose **Sketch → Export Compiled Binary** with the same board and OTA partition scheme used for the initial flash. The sketch folder will contain the compiled `.bin` image. Use the application `.bin`, not a bootloader, partition-table, merged, or filesystem image.
3. Upload the `.bin` to an HTTPS release host. Keep its URL stable in the manifest and make sure the host is covered by `OTA_ROOT_CA`.
4. Calculate the image byte length and SHA-256 digest, then publish a `version.json` next to the image. Example:

```json
{
  "version": "1.0.1",
  "url": "https://your-host.example/releases/rgb-lamp-1.0.1.bin",
  "sha256": "64-character-SHA-256-hex-digest",
  "size": 1234567
}
```

5. Open the lamp controller, choose **Check for update**, then **Install update**. Keep power connected while the image downloads, verifies, and the lamp restarts.

The manifest version must be a newer three-part numeric version (`major.minor.patch`). Firmware images are capped at 3 MiB and must fit the inactive app slot. The currently configured version is `1.0.0`; the URL and CA are intentionally placeholders until the release host is chosen.

## Endpoints

- `GET /ota/status` — installed version, partition support, state, and progress.
- `POST /ota/check` — fetches and validates the HTTPS manifest.
- `POST /ota/update` — downloads the image, checks its length and SHA-256, writes the inactive slot, and restarts after a successful update.

The sketch currently uses the ESP32 Arduino core's built-in OTA slot selection. Automatic rollback requires enabling/configuring bootloader rollback in the board's build configuration; that is not assumed here.
