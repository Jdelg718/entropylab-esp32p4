# Recovery — no downloadable recovery image yet

This candidate has no published, tested recovery binary. Preserve an independently verified known-good source/build mapping and application hash before an authorized update. Do not publish device factory backups or use another owner's backup.

If flashing is interrupted, disconnect the serial monitor, check the USB TO UART connector and data cable, and use the board's documented BOOT/RESET download-mode sequence. Retry only the same verified release and the same approved installation mode once identity and compatibility are established. A failed app-only update is not permission to replace the partition table. If bootloader or layout compatibility is unknown, stop and obtain board-specific review rather than escalating to erase or force.

Never erase the whole chip, modify eFuses, bypass revision/security checks, change secure boot/flash encryption, or flash the companion C6 as an improvised recovery. No actual interruption/recovery test or rollback validation has been performed for this candidate. See [BUILD-FLASH.md](BUILD-FLASH.md).
