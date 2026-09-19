> **Current public install profile:** exact Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, PCB rev1.3, 32 MiB; human PCB confirmation is required. Silicon 100–199 acceptance is necessary but does not prove PCB identity or support for every Rev1.x board. See [first-install guide](../../PUBLIC-FIRST-INSTALL.md). Historical observations below are not a broad PCB matrix.

# Supported hardware and official evidence

## Current supported target

EntropyLab currently targets one verified configuration:

- Waveshare `ESP32-P4-WIFI6-Touch-LCD-4.3`;
- 4.3-inch portrait IPS display, 480 × 800;
- physical ESP32-P4 Rev1.3 test board;
- target configuration `CONFIG_ESP32P4_REV_MIN_100` with 200 MHz PSRAM;
- accepted image revision range 1.0 through 1.99.

This does not claim support for Rev3.x silicon, another Waveshare screen size, or a board that merely has a similar name.

## Product variants and bundle names

Waveshare's official documentation lists:

- SKU 33874 — `ESP32-P4-WIFI6-Touch-LCD-4.3`, standard version;
- SKU 33875 — `ESP32-P4-WIFI6-Touch-LCD-4.3-C`, supplied with the optional OV5647 camera.

Some resellers use labels such as “Package C.” That is a seller bundle label, not an ESP32-P4 revision or an EntropyLab compatibility profile. Check the actual product/SKU and silicon revision. Do not infer Rev1.3 or camera presence from the bundle letter.

## Verified board details

Official Waveshare product documentation describes:

- ESP32-P4NRW32 and 32 MB external NOR flash;
- 32 MB in-package PSRAM;
- ESP32-C6-MINI-1 wireless coprocessor over SDIO;
- 4.3-inch 480 × 800 IPS panel, ST7701, 2-lane MIPI-DSI;
- GT911 capacitive touch, up to five points depending on software;
- USB-to-UART Type-C for power/programming/debugging;
- separate USB OTG 2.0 High-Speed Type-C;
- BOOT, RESET, and POWER buttons;
- TF/microSD slot, 15-pin 0.5 mm MIPI-CSI camera connector, GH1.25 2-pin speaker header, battery headers, and 40-pin expansion header.

The official quick package overview lists the board and an 8 Ω 2 W speaker, with the OV5647 camera optional. It does not list a USB cable. EntropyLab requires the board and a known-good USB **data** cable. The camera, speaker, batteries, TF card, and header adapter are not required by the current application.

Use the port labeled **USB TO UART** for the documented programming/debugging path. Do not confuse it with the adjacent USB OTG port.

## Revision evidence

The Waveshare FAQ documents real boards reporting ESP32-P4 revision v1.3 and warns against forcing a Rev3-targeted bootloader onto them. The accepted EntropyLab source has an explicit `rev1_3.defaults` overlay:

```text
CONFIG_ESP32P4_REV_MIN_100=y
CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y
CONFIG_SPIRAM_SPEED_200M=y
```

ESP-IDF's `REV_MIN_100` is the available v1.0 floor for the Rev1.x family; it is not a claim that the board is exactly Rev1.0. The accepted target verifier reported `revision_min_full=100` and `revision_max_full=199`. Never use `--force` to bypass a revision mismatch. Stop and identify the board instead.

## Official references

- Waveshare product documentation: https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-4.3
- Waveshare official store/product page: https://www.waveshare.com/esp32-p4-wifi6-touch-lcd-4.3.htm
- Waveshare FAQ: https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-4.3/FAQ
- Waveshare official source/examples: https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3
- Espressif ESP-IDF v5.5.5 serial connection guide: https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32p4/get-started/establish-serial-connection.html

These references describe the hardware and generic tooling. EntropyLab's own generated build metadata remains the authority for its flash artifacts and layout.
