# ESP32-S3 RGB Lamp with signed OTA

Arduino firmware for a one-pixel RGB lamp. It keeps Wi-Fi setup and recovery through a SoftAP portal, stores credentials in NVS, retries the saved network, and serves the RGB controller and OTA controls.

## Board and partition settings

The current build targets **ESP32S3 Dev Module** with ESP32 Arduino core **3.3.7**, 16 MB flash, OPI PSRAM, and the **16M FatFS / OTA** partition scheme (`PartitionScheme=fatflash`). That scheme provides two 2 MiB OTA application slots. The application image must stay within the inactive slot.

A device whose current partition table has no OTA slot needs one USB upload with this OTA-capable partition scheme. Repartitioning requires a USB upload and may erase flash data. Later updates can use the controller's HTTPS OTA page.

## Build and test

A GitHub Actions workflow compiles the sketch on pushes and pull requests with the board settings above. It installs pinned dependencies: Adafruit NeoPixel 1.15.4 and ArduinoJson 7.4.3. The workflow also checks that the application image is no larger than 2 MiB.

To build locally in Arduino IDE, open `RGBsetup/RGBsetup.ino`, select **ESP32S3 Dev Module**, and match the settings above, then choose **Sketch → Verify/Compile**. For an OTA release, use the same settings and the application `.bin` only; do not publish the bootloader, partition table, filesystem, or merged image.

## Versioned GitHub releases

The release workflow runs for a tag named `vMAJOR.MINOR.PATCH` (for example `v1.0.5`) and requires the tag version to match `FIRMWARE_VERSION` in the sketch. It compiles the firmware, checks the OTA slot size, calculates SHA-256 and byte length, signs the manifest using the GitHub Actions secret `OTA_SIGNING_PRIVATE_KEY_PEM`, attaches `rgb-lamp-vMAJOR.MINOR.PATCH.bin` and `version.json` to a GitHub Release, then updates the root `version.json` manifest on `main`.

To publish a release:

1. Set `FIRMWARE_VERSION` to the next three-part version in `RGBsetup/RGBsetup.ino` and push that source change to `main`.
2. Create and push the matching version tag, such as `v1.0.5`.
3. Confirm the **Firmware build and release** workflow succeeds in the repository's Actions tab.

The device reads the manifest over HTTPS, verifies its RSA-2048/SHA-256 signature against the public key in the sketch, accepts only the repository release URL matching the signed version, then downloads firmware over HTTPS and checks its size and SHA-256 before writing the inactive OTA slot. HTTPS certificate checks remain enabled. The signing private key must be stored only as a repository Actions secret named `OTA_SIGNING_PRIVATE_KEY_PEM`; never commit it.

## v1.0.5 reliability and OTA behavior

- A normal Wi-Fi disconnect starts exponential reconnect attempts, capped at 60 seconds. After two minutes, the device also starts the existing recovery access point; the RGB lamp and controller continue running. When saved Wi-Fi returns, the recovery access point closes.
- RGB color and on/off state are saved to NVS after a one-second quiet period, avoiding a flash write for every slider event.
- Firmware downloads run in bounded chunks from the main loop. The controller can poll `/ota/status` for live progress while the download continues.
- The application checks for `ESP_OTA_IMG_PENDING_VERIFY` and marks the new image valid only after the web server has been running for 15 seconds. The workflow checks that the pinned ESP32-S3 SDK profile enables bootloader rollback.
- Signed manifest verification, HTTPS validation, release URL restrictions, image-size checks, and SHA-256 verification are retained.

The board's installed bootloader must also support rollback for Phase 3 to work on hardware; an OTA application update cannot replace a bootloader. Confirm the first v1.0.5 update and rollback behavior on the actual board before relying on it.

## Remaining device-security setup

This firmware does not enable Secure Boot, flash encryption, or encrypted NVS, and it does not add login authentication to the local setup/controller pages. The current setup AP password is shared in the public source. These settings need a deliberate hardware provisioning and credential migration plan; enabling eFuse protections can limit recovery and is not done by the OTA release workflow. Keep the lamp powered while installing an update.
