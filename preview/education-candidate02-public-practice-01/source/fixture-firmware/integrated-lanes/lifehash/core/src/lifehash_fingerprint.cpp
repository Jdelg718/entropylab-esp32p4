#include "lifehash_fingerprint.h"
#include "lifehash.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace {

bool overlaps(const uint8_t* input, uint8_t* output) noexcept {
    const auto in = reinterpret_cast<uintptr_t>(input);
    const auto out = reinterpret_cast<uintptr_t>(output);
    if (in <= out) {
        return out - in < 4;
    }
    return in - out < LIFEHASH_FINGERPRINT_RGB_SIZE;
}

} // namespace

extern "C" LifeHashFingerprintStatus lifehash_fingerprint_render(
    const uint8_t* fingerprint,
    size_t fingerprint_size,
    uint32_t version,
    uint8_t* rgb_out,
    size_t rgb_capacity) {
    if (fingerprint == nullptr) {
        return LIFEHASH_FINGERPRINT_NULL_INPUT;
    }
    if (fingerprint_size != 4) {
        return LIFEHASH_FINGERPRINT_INVALID_INPUT_SIZE;
    }
    if (rgb_out == nullptr) {
        return LIFEHASH_FINGERPRINT_NULL_OUTPUT;
    }
    if (rgb_capacity < LIFEHASH_FINGERPRINT_RGB_SIZE) {
        return LIFEHASH_FINGERPRINT_OUTPUT_TOO_SMALL;
    }
    if (version != LIFEHASH_FINGERPRINT_VERSION_2) {
        return LIFEHASH_FINGERPRINT_UNSUPPORTED_VERSION;
    }
    if (overlaps(fingerprint, rgb_out)) {
        return LIFEHASH_FINGERPRINT_ALIASED_BUFFERS;
    }

    try {
        const std::vector<uint8_t> input(fingerprint, fingerprint + 4);
        const auto image = LifeHash::make_from_data(
            input, LifeHash::Version::version2, 1, false);
        if (image.width != LIFEHASH_FINGERPRINT_WIDTH ||
            image.height != LIFEHASH_FINGERPRINT_HEIGHT ||
            image.colors.size() != LIFEHASH_FINGERPRINT_RGB_SIZE) {
            return LIFEHASH_FINGERPRINT_INTERNAL_ERROR;
        }

        std::array<uint8_t, LIFEHASH_FINGERPRINT_RGB_SIZE> completed{};
        std::copy(image.colors.begin(), image.colors.end(), completed.begin());
        std::copy(completed.begin(), completed.end(), rgb_out);
        return LIFEHASH_FINGERPRINT_OK;
    } catch (...) {
        return LIFEHASH_FINGERPRINT_INTERNAL_ERROR;
    }
}
