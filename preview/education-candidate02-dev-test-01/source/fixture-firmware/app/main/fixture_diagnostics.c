// Diagnostic candidate only. No message, operands, input data, or addresses.
// ROM printing avoids the normal logger's locks and heap allocation.
#include <stddef.h>
#include <stdint.h>
#include "esp_attr.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "multi_heap.h"

// Private internal DRAM, not registered in the shared caps allocator: GUI
// malloc must not consume the Rust computation budget. No PSRAM fallback.
// Two compute workers share this pool under the IDF recursive heap lock.
// Public D6 RAW24+empty host measurement: 384 live payload bytes per worker.
// 8 KiB includes TLSF metadata/alignment and headroom for other accepted inputs.
static DRAM_ATTR _Alignas(16) uint8_t fixture_rust_memory[8192];
static multi_heap_handle_t fixture_rust_heap;
static portMUX_TYPE fixture_rust_lock = portMUX_INITIALIZER_UNLOCKED;

// Call once from app_main, before workers or GUI can invoke Rust.
int fixture_allocator_init(void) {
    if (fixture_rust_heap) return 1;
    fixture_rust_heap = multi_heap_register(fixture_rust_memory, sizeof fixture_rust_memory);
    if (!fixture_rust_heap) return 0;
    multi_heap_set_lock(fixture_rust_heap, &fixture_rust_lock);
    return 1;
}

void *fixture_alloc(size_t n, size_t align) {
    if (align < sizeof(void *)) align = sizeof(void *);
    void *p = fixture_rust_heap ? multi_heap_aligned_alloc(fixture_rust_heap, n ? n : 1, align) : NULL;
    if (!p) {
        multi_heap_info_t info = {0};
        if (fixture_rust_heap) multi_heap_get_info(fixture_rust_heap, &info);
        const size_t free_internal = info.total_free_bytes;
        const size_t largest = info.largest_free_block;
        // ESP32-P4 size_t is 32 bits; ROM printf supports unsigned decimal.
        esp_rom_printf("EL_DIAG_ALLOC_FAIL size=%u align=%u free=%u largest=%u\n",
                       (unsigned)n, (unsigned)align, (unsigned)free_internal, (unsigned)largest);
    }
    return p;
}
void fixture_free(void *p) { if (p) multi_heap_free(fixture_rust_heap, p); }

// file is only a Rust compiler-provided static source location, never user text.
// Fixed stack storage; ASCII allowlist; no complete paths or panic formatting.
void fixture_panic_location(const uint8_t *file, size_t len, uint32_t line) {
    char basename[64];
    size_t start = 0, used = 0;
    if (file) {
        for (size_t i = 0; i < len; ++i)
            if (file[i] == '/' || file[i] == '\\') start = i + 1;
        for (size_t i = start; i < len && used < sizeof(basename) - 1; ++i) {
            const uint8_t c = file[i];
            basename[used++] = ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                                (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.') ? (char)c : '_';
        }
    }
    if (!used) basename[used++] = '_';
    basename[used] = '\0';
    esp_rom_printf("EL_DIAG_PANIC file=%s line=%u\n", basename, (unsigned)line);
}
