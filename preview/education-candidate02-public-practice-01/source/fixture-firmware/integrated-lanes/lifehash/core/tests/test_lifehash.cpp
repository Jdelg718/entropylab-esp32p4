#include "lifehash_fingerprint.h"
#include "lifehash.hpp"
#include "lifehash.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::string sha256_hex(const uint8_t* data, size_t size) {
    uint8_t digest[32];
    lifehash_sha256(data, size, digest);
    static const char hex[] = "0123456789abcdef";
    std::string result(64, '0');
    for (size_t i = 0; i < 32; ++i) {
        result[i * 2] = hex[digest[i] >> 4];
        result[i * 2 + 1] = hex[digest[i] & 15];
    }
    return result;
}

std::array<uint8_t, 4> bytes(uint32_t value) {
    return {static_cast<uint8_t>(value >> 24), static_cast<uint8_t>(value >> 16),
            static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value)};
}

std::array<uint8_t, LIFEHASH_FINGERPRINT_RGB_SIZE> render(uint32_t fingerprint) {
    const auto input = bytes(fingerprint);
    std::array<uint8_t, LIFEHASH_FINGERPRINT_RGB_SIZE> output{};
    const auto status = lifehash_fingerprint_render(
        input.data(), input.size(), LIFEHASH_FINGERPRINT_VERSION_2,
        output.data(), output.size());
    check(status == LIFEHASH_FINGERPRINT_OK, "valid render succeeds");
    return output;
}

void test_golden_vectors() {
    struct Vector { uint32_t input; const char* expected; const char* origin; };
    const Vector vectors[] = {
        {0x73c5da0a, "09da10ffd57a4f58616a5eda313d3f0c861e79b93e1b609a012f9c3530b427b5", "EntropyLab public fixture"},
        {0x00000000, "9003d9fd366ec3aa06f54d6797485114ec00c61bf85c0efafa91bd2e40176d5b", "EntropyLab public fixture"},
        {0xffffffff, "e856f1b33dfd8eef83151de7407c3d4861581ce09f11f11f2dfc6b0219a1e51b", "EntropyLab public fixture"},
        {0xb8688df1, "d44ba038c1389003c955a6f17accfb87c98fce4e8c98c9e2a44c71067b6521fe", "EntropyLab public fixture"},
        {0xdeadbeef, "cb5c61fdbab952cd54b86824291d14e36255df58c80d25f7463db369e2d1ccf6", "bc-lifehash upstream raw-byte vector"},
    };
    for (const auto& vector : vectors) {
        const auto output = render(vector.input);
        const auto actual = sha256_hex(output.data(), output.size());
        check(actual == vector.expected, vector.origin);
    }
}

void test_contract_and_no_partial_output() {
    const auto input = bytes(0x73c5da0a);
    std::array<uint8_t, LIFEHASH_FINGERPRINT_RGB_SIZE + 2> guarded;
    guarded.fill(0xa5);

    auto expect_failure_unchanged = [&](LifeHashFingerprintStatus expected,
                                        const uint8_t* in, size_t in_size,
                                        uint32_t version, uint8_t* out,
                                        size_t capacity, const char* message) {
        const auto before = guarded;
        const auto status = lifehash_fingerprint_render(in, in_size, version, out, capacity);
        check(status == expected, message);
        check(guarded == before, "failure leaves output untouched");
    };

    expect_failure_unchanged(LIFEHASH_FINGERPRINT_NULL_INPUT, nullptr, 4,
                             LIFEHASH_FINGERPRINT_VERSION_2, guarded.data() + 1,
                             LIFEHASH_FINGERPRINT_RGB_SIZE, "null input rejected");
    expect_failure_unchanged(LIFEHASH_FINGERPRINT_INVALID_INPUT_SIZE, input.data(), 3,
                             LIFEHASH_FINGERPRINT_VERSION_2, guarded.data() + 1,
                             LIFEHASH_FINGERPRINT_RGB_SIZE, "short input rejected");
    expect_failure_unchanged(LIFEHASH_FINGERPRINT_INVALID_INPUT_SIZE, input.data(), 5,
                             LIFEHASH_FINGERPRINT_VERSION_2, guarded.data() + 1,
                             LIFEHASH_FINGERPRINT_RGB_SIZE, "long input rejected");
    expect_failure_unchanged(LIFEHASH_FINGERPRINT_UNSUPPORTED_VERSION, input.data(), 4,
                             1, guarded.data() + 1, LIFEHASH_FINGERPRINT_RGB_SIZE,
                             "non-version2 rejected");
    expect_failure_unchanged(LIFEHASH_FINGERPRINT_OUTPUT_TOO_SMALL, input.data(), 4,
                             LIFEHASH_FINGERPRINT_VERSION_2, guarded.data() + 1,
                             LIFEHASH_FINGERPRINT_RGB_SIZE - 1, "small output rejected");

    check(lifehash_fingerprint_render(input.data(), 4, LIFEHASH_FINGERPRINT_VERSION_2,
                                      nullptr, LIFEHASH_FINGERPRINT_RGB_SIZE) ==
              LIFEHASH_FINGERPRINT_NULL_OUTPUT,
          "null output rejected");

    const auto status = lifehash_fingerprint_render(
        input.data(), 4, LIFEHASH_FINGERPRINT_VERSION_2,
        guarded.data() + 1, LIFEHASH_FINGERPRINT_RGB_SIZE);
    check(status == LIFEHASH_FINGERPRINT_OK, "guarded render succeeds");
    check(guarded.front() == 0xa5 && guarded.back() == 0xa5, "output canaries preserved");

    const std::array<uint8_t, 6> guarded_input{
        0x5a, input[0], input[1], input[2], input[3], 0xc3};
    const auto input_before = guarded_input;
    std::array<uint8_t, LIFEHASH_FINGERPRINT_RGB_SIZE> input_canary_output{};
    check(lifehash_fingerprint_render(
              guarded_input.data() + 1, 4, LIFEHASH_FINGERPRINT_VERSION_2,
              input_canary_output.data(), input_canary_output.size()) ==
              LIFEHASH_FINGERPRINT_OK,
          "input-canary render succeeds");
    check(guarded_input == input_before, "input bytes and canaries remain unchanged");

    std::array<uint8_t, LIFEHASH_FINGERPRINT_RGB_SIZE> overlap{};
    std::memcpy(overlap.data() + 8, input.data(), input.size());
    const auto overlap_before = overlap;
    check(lifehash_fingerprint_render(overlap.data() + 8, 4,
                                      LIFEHASH_FINGERPRINT_VERSION_2,
                                      overlap.data(), overlap.size()) ==
              LIFEHASH_FINGERPRINT_ALIASED_BUFFERS,
          "input/output overlap rejected");
    check(overlap == overlap_before, "overlap rejection leaves output untouched");
}

void test_serialization_negative_controls() {
    const auto canonical = render(0x73c5da0a);
    const std::vector<uint8_t> ascii{'7','3','c','5','d','a','0','a'};
    const auto ascii_image = LifeHash::make_from_data(
        ascii, LifeHash::Version::version2, 1, false);
    check(ascii_image.colors != std::vector<uint8_t>(canonical.begin(), canonical.end()),
          "ASCII hex does not match raw fingerprint bytes");

    const auto input = bytes(0x73c5da0a);
    uint8_t digest[32];
    lifehash_sha256(input.data(), input.size(), digest);
    const std::vector<uint8_t> prehashed(digest, digest + sizeof(digest));
    const auto double_hashed = LifeHash::make_from_data(
        prehashed, LifeHash::Version::version2, 1, false);
    check(double_hashed.colors != std::vector<uint8_t>(canonical.begin(), canonical.end()),
          "prehashed data path does not match exactly-once hashing");
}

void test_determinism_and_render() {
    const auto first = render(0x73c5da0a);
    const auto second = render(0x73c5da0a);
    check(first == second, "repeated calls are deterministic");

    std::ofstream ppm("evidence/lifehash-73c5da0a.ppm", std::ios::binary);
    ppm << "P6\n32 32\n255\n";
    ppm.write(reinterpret_cast<const char*>(first.data()), first.size());
    check(ppm.good(), "actual module output written to PPM");

    const auto upstream = render(0xdeadbeef);
    std::ofstream rgb("evidence/lifehash-deadbeef.rgb", std::ios::binary);
    rgb.write(reinterpret_cast<const char*>(upstream.data()), upstream.size());
    check(rgb.good(), "upstream raw-byte vector output written for byte comparison");
}

} // namespace

int main() {
    test_golden_vectors();
    test_contract_and_no_partial_output();
    test_serialization_negative_controls();
    test_determinism_and_render();
    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "PASS: LifeHash wrapper vectors, boundary cases, negative controls, canaries, determinism, and render\n";
    return EXIT_SUCCESS;
}
