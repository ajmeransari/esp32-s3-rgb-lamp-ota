# ESP32-S3 RGB Lamp with secure OTA

Arduino firmware for a one-pixel RGB lamp. It keeps Wi-Fi setup and recovery through a SoftAP portal, stores credentials in NVS, retries the saved network, and serves the RGB controller and OTA controls.

## Board and partition settings

The current build targets **ESP32S3 Dev Module** with ESP32 Arduino core **3.3.7**, 16 MB flash, OPI PSRAM, and the **16M FatFS / OTA** partition scheme (`PartitionScheme=fatflash`). That scheme provides two 2 MiB OTA application slots. The application image must stay within the inactive slot.

A device whose current partition table has no OTA slot needs one USB upload with this OTA-capable partition scheme. Repartitioning requires a USB upload and may erase flash data. Later updates can use the controller's HTTPS OTA page.

## Build and test

A GitHub Actions workflow compiles the sketch on pushes and pull requests with the board settings above. It installs pinned dependencies: Adafruit NeoPixel 1.15.4 and ArduinoJson 7.4.3. The workflow also checks that the application image is no larger than 2 MiB.

To build locally in Arduino IDE, open `RGBsetup/RGBsetup.ino`, select **ESP32S3 Dev Module**, and match the settings above, then choose **Sketch → Verify/Compile**. For an OTA release, use the same settings and the application `.bin` only; do not publish the bootloader, partition table, filesystem, or merged image.

## Versioned GitHub releases

The release workflow runs for a tag named `vMAJOR.MINOR.PATCH` (for example `v1.0.0`) and requires the tag version to match `FIRMWARE_VERSION` in the sketch. It compiles the firmware, checks the OTA slot size, calculates SHA-256 and byte length, attaches `rgb-lamp-vMAJOR.MINOR.PATCH.bin` and `version.json` to a GitHub Release, then updates the root `version.json` manifest on `main`.

To publish a release:

1. Set `FIRMWARE_VERSION` to the next three-part version in `RGBsetup/RGBsetup.ino` and push that source change to `main`.
2. Create and push the matching version tag, such as `v1.0.1`.
3. Confirm the **Firmware build and release** workflow succeeds in the repository's Actions tab.

The device reads the manifest over HTTPS from `raw.githubusercontent.com`, downloads the firmware over HTTPS, checks the manifest version, size, and SHA-256, then writes the inactive OTA slot. HTTPS certificate checks remain enabled.

## Preserved lamp behavior

The sketch retains the existing Wi-Fi provisioning and recovery page, NVS-stored credentials, connection retry/fallback, and RGB controls. OTA is exposed in the controller and through `/ota/status`, `/ota/check`, and `/ota/update`.

Automatic rollback is not enabled by this project. Keep the lamp powered while installing an update. The first USB upload and a real over-the-air update still need to be tested on the actual board.