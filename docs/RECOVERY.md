# Recovery before hardware experiments

Never flash a board containing real wallet material. Arrange exclusive device
ownership and explicit flashing authorization. Use a disposable development board.

1. Identify the board revision, flash size and correct USB interface locally.
   Do not publish its serial number, hardware addresses, host names or port paths.
2. Before changes, use the matching esptool/IDF tooling to read the complete flash
   into a private directory OUTSIDE this checkout. Confirm size, compute a checksum,
   and preserve two private copies. A dump may contain credentials or secrets.
3. Verify app AND bootloader revision bounds, flash offsets from the generated
   flasher metadata, and binary checksums. Never guess offsets or reuse a different
   board's dump. Rev1.3 requires the reviewed legacy PHY / 200 MHz PSRAM profile.
4. Follow the board manufacturer's BOOT/RESET download-mode procedure if necessary.
   Restore only the previously verified image for that same board using its
   recorded flash settings. Do not change eFuses or enable irreversible security
   settings as troubleshooting steps.
5. Reboot and verify display, touch, PSRAM and fixture computation. Sanitize any
   evidence manually before proposing it for publication.

No flash dump, recovery binary, raw serial transcript, device identifier or private
backup is included or accepted in this repository. If recovery cannot be verified,
stop and consult the board/ESP-IDF documentation rather than experimenting blindly.
