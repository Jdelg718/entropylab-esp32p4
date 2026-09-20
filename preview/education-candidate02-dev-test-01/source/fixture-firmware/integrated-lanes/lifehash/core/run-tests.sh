#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"
export LC_ALL=C
export CXX="${CXX:-g++}"

rm -rf build evidence
mkdir -p build evidence

cmake -S . -B build/normal -DCMAKE_BUILD_TYPE=Release
cmake --build build/normal --parallel 2
ctest --test-dir build/normal --output-on-failure

python3 measure.py build/normal/lifehash_tests evidence/linux-host-resources.json
python3 verify.py

if printf 'int main(){return 0;}\n' | "$CXX" -x c++ -fsanitize=address,undefined -o build/sanitizer-probe - >/dev/null 2>&1; then
    cmake -S . -B build/sanitized -DCMAKE_BUILD_TYPE=Debug -DLIFEHASH_ENABLE_SANITIZERS=ON
    cmake --build build/sanitized --parallel 2
    ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
        ctest --test-dir build/sanitized --output-on-failure
    printf '%s\n' 'PASS: AddressSanitizer and UndefinedBehaviorSanitizer' > evidence/sanitizer-status.txt
else
    printf '%s\n' 'SKIP: compiler does not support requested sanitizers' > evidence/sanitizer-status.txt
fi

sha256sum evidence/* > evidence/SHA256SUMS
printf '%s\n' 'PASS: standalone LifeHash normal and available sanitizer validation complete'
