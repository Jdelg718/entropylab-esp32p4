> **Current guidance:** [RELEASE-BUILD-HYGIENE — current retention preview](current/docs/RELEASE-BUILD-HYGIENE.md).
> The text below is the historical source checkpoint, not current release status.
> v0.1.0-retention-preview is published; v0.1.1 is a local documentation/onboarding patch pending publication.
> Firmware is unchanged; public-policy hardware qualification and the separate retention updater remain held.

# Release recipe h2 — configure/object candidate; distribution HOLD

Build only a fresh regular-file tree authenticated by PACKAGE-SOURCE-MANIFEST.json.
Validation rejects changed bytes, extra/missing files and special nodes before generation.
The descriptor is `s<first 24 hex of manifest SHA256>-h2`; record the complete digest.
It is not a Git revision, release version or reproducibility proof.

Source maps to /src/entropylab, IDF to /IDF, Cargo home to /deps/cargo,
and the pinned Rust sysroot to /toolchain/rust. Lexical/resolved roots and
leading-double-slash spellings are mapped; longest prefixes come last.
The pinned GCC gives file-prefix maps precedence over macro-prefix maps.
Generated LifeHash allocated literals may retain a double leading slash in the
public destination (`//src/entropylab/...`); this is recorded, not normalized away.
Unsafe whitespace, equals, quotes, backslashes and semicolons in roots fail closed.

IDF 5.5.5 initializes compiler flags before ordinary CFLAGS inheritance can help.
h2 explicitly appends file-prefix maps through idf_toolchain_add_flags to the
application's real compiler response files and forwards CMAKE_C/CXX/ASM_FLAGS
through EXTRA_CMAKE_ARGS to the independent bootloader configure.
The launcher gates both actual Ninja/compile_commands graphs before target build,
checking coverage, ordered maps, ABI/architecture, assertions and response files.
The application has C/C++/ASM; the observed bootloader has C only. All three
bootloader language flag files are still checked. Cargo C and Rust receive their
own explicit flags; arbitrary hand-written string literals are not rewritten.

Public BIN/runtime constraints and private debug evidence are distinct. Scan
all allocated ELF/object sections for private paths and inspect full BIN files.
Preserve and classify nonallocated debug hits rather than hiding or stripping them.
The real LifeHash object retains GCC C++ include paths in nonallocated DWARF:
ELF/object/MAP/log/graph artifacts remain PRIVATE and are not approved for public
publication. This recipe does not promise all debug toolchain paths are remapped.
Do not strip, hex-patch, disable assertions/panic safety or suppress warnings.

The available evidence is real configure graphs plus a LifeHash object probe,
not a complete target rebuild, complete BIN scan, hardware PASS or release.
After independent review, run the isolated complete firmware build; verify
source identity, unchanged locks, descriptor, ABI, partition/image integrity,
and app/bootloader runtime/BIN paths. Timestamp remains build time.
Distribution remains HOLD. The predecessor's empty .hermes-tmp files are retained.
