#ifndef ENTROPYLAB_LIFEHASH_FINGERPRINT_H
#define ENTROPYLAB_LIFEHASH_FINGERPRINT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    LIFEHASH_FINGERPRINT_WIDTH = 32,
    LIFEHASH_FINGERPRINT_HEIGHT = 32,
    LIFEHASH_FINGERPRINT_RGB_SIZE = 32 * 32 * 3,
    LIFEHASH_FINGERPRINT_VERSION_2 = 2
};

typedef enum LifeHashFingerprintStatus {
    LIFEHASH_FINGERPRINT_OK = 0,
    LIFEHASH_FINGERPRINT_NULL_INPUT = 1,
    LIFEHASH_FINGERPRINT_INVALID_INPUT_SIZE = 2,
    LIFEHASH_FINGERPRINT_NULL_OUTPUT = 3,
    LIFEHASH_FINGERPRINT_OUTPUT_TOO_SMALL = 4,
    LIFEHASH_FINGERPRINT_UNSUPPORTED_VERSION = 5,
    LIFEHASH_FINGERPRINT_ALIASED_BUFFERS = 6,
    LIFEHASH_FINGERPRINT_INTERNAL_ERROR = 7
} LifeHashFingerprintStatus;

/*
 * Render exactly four raw BIP32 master-fingerprint bytes. This is never ASCII
 * hex and never a precomputed digest. The implementation uses LifeHash
 * make_from_data semantics (SHA-256 once), version2, module_size=1, RGB only.
 *
 * On every failure, rgb_out is left untouched. Buffers must not overlap.
 * Capacity may exceed RGB_SIZE, but exactly RGB_SIZE bytes are written.
 */
LifeHashFingerprintStatus lifehash_fingerprint_render(
    const uint8_t* fingerprint,
    size_t fingerprint_size,
    uint32_t version,
    uint8_t* rgb_out,
    size_t rgb_capacity);

#ifdef __cplusplus
}
#endif

#endif
