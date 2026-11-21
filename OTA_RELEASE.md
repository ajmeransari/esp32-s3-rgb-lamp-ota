# ESP32-S3 RGB Lamp OTA release guide

## Board configuration

Use **ESP32S3 Dev Module**, ESP32 Arduino core **3.3.7**, 16 MB flash, OPI PSRAM, and the **16M FatFS / OTA** partition scheme (`PartitionScheme=fatflash`). This layout has two 2 MiB OTA application slots. The sketch caps firmware downloads at 2 MiB and rejects an image whose size or SHA-256 does not match the manifest.

If the lamp was previously flashed with a partition table that lacks OTA slots, connect USB and upload once using this OTA-capable scheme. This repartitioning upload may erase data. A later OTA update cannot change the partition table.

## Publish a version

1. Change `FIRMWARE_VERSION` in `RGBsetup/RGBsetup.ino` to the next semantic version such as `1.0.1` and push the change to `main`.
2. Create a tag with the same number prefixed by `v`, for example `v1.0.1`, and push it.
3. The **Firmware build and release** GitHub Actions workflow compiles using the pinned board/core/library settings, rejects images above 2 MiB, computes the binary size and SHA-256, and creates a GitHub Release.
4. The release contains the application image `rgb-lamp-v1.0.1.bin` plus `version.json`. The workflow also updates the repository root `version.json`, which the lamp reads from the configured HTTPS manifest URL.
5. Check the Actions run and public release page before asking the lamp to check/install the update.

The tag version must exactly match `FIRMWARE_VERSION`. Use only the application `.bin`; do not use bootloader, partition table, filesystem, or merged image files. The GitHub Actions build is the versioned release build. A successful compile does not replace a real hardware OTA test.

## Security and behavior

OTA requires Wi-Fi connectivity, HTTPS certificate validation, a valid three-part manifest version newer than the installed firmware, a firmware size in the declared limit, and a matching SHA-256 digest. The sketch has a pinned trust bundle for the GitHub HTTPS hosts; rotate it only after validating the replacement certificates. No TLS verification bypass is used.

Existing SoftAP Wi-Fi provisioning, NVS credentials, connection retry and recovery fallback, setup/recovery page, and RGB controller remain in the sketch. Status and actions are available through `/ota/status`, `/ota/check`, and `/ota/update`. Automatic bootloader rollback is not configured.