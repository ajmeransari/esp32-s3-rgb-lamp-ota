# ESP32-S3 RGB Lamp with signed OTA

Arduino firmware for a one-pixel RGB lamp. It keeps Wi-Fi setup and recovery through a SoftAP portal, stores credentials in NVS, retries the saved network, and serves the RGB controller and OTA controls.

## Board and partition settings

The current build targets **ESP32S3 Dev Module** with ESP32 Arduino core **3.3.7**, 16 MB flash, OPI PSRAM, and the **16M FatFS / OTA** partition scheme (`PartitionScheme=fatflash`). That scheme provides two 2 MiB OTA application slots. The application image must stay within the inactive slot.

A device whose current partition table has no OTA slot needs one USB upload with this OTA-capable partition scheme. Repartitioning requires a USB upload and may erase flash data. Later updates can use the controller's HTTPS OTA page.

## Build and test

A GitHub Actions workflow compiles the sketch on pushes and pull requests with the board settings above. It installs pinned dependencies: Adafruit NeoPixel 1.15.4 and ArduinoJson 7.4.3. The workflow also checks that the application image is no larger than 2 MiB.

To build locally in Arduino IDE, open `RGBsetup/RGBsetup.ino`, select **ESP32S3 Dev Module**, and match the settings above, then choose **Sketch → Verify/Compile**. For an OTA release, use the same settings and the application `.bin` only; do not publish the bootloader, partition table, filesystem, or merged image.

## Versioned GitHub releases

The release workflow runs for a tag named `vMAJOR.MINOR.PATCH` (for example `v1.0.3`) and requires the tag version to match `FIRMWARE_VERSION` in the sketch. It compiles the firmware, checks the OTA slot size, calculates SHA-256 and byte length, signs the manifest using the GitHub Actions secret `OTA_SIGNING_PRIVATE_KEY_PEM`, attaches `rgb-lamp-vMAJOR.MINOR.PATCH.bin` and `version.json` to a GitHub Release, then updates the root `version.json` manifest on `main`.

To publish a release:

1. Set `FIRMWARE_VERSION` to the next three-part version in `RGBsetup/RGBsetup.ino` and push that source change to `main`.
2. Create and push the matching version tag, such as `v1.0.3`.
3. Confirm the **Firmware build and release** workflow succeeds in the repository's Actions tab.

The device reads the manifest over HTTPS, verifies its RSA-2048/SHA-256 signature against the public key in the sketch, accepts only the repository release URL matching the signed version, then downloads firmware over HTTPS and checks its size and SHA-256 before writing the inactive OTA slot. HTTPS certificate checks remain enabled. The signing private key must be stored only as a repository Actions secret named `OTA_SIGNING_PRIVATE_KEY_PEM`; never commit it.

## Preserved lamp behavior

The sketch retains the existing Wi-Fi provisioning and recovery page, NVS-stored credentials, connection retry/fallback, and RGB controls. OTA is exposed in the controller and through `/ota/status`, `/ota/check`, and `/ota/update`.

Automatic bootloader rollback, Secure Boot, flash encryption, encrypted NVS, and authentication for the local setup/controller pages are not configured by this sketch. The existing setup AP password is a shared value in the source and should be changed before public deployment. Keep the lamp powered while installing an update. Version 1.0.3 must pass GitHub Actions and a real hardware OTA test before it is treated as a completed release.