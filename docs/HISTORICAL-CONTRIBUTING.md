> Historical education/docs147 contribution checkpoint; old branch and failure claims below are not current guidance. See [current status](CURRENT-STATUS.md). Embedded paths use repository root.

## Check out the publication branch

The proposed dedicated branch is `release/education-ui-20260917-docs147`. It is not on the public remote until the exact package passes independent publication review and is pushed. After publication:

```sh
git clone https://github.com/Jdelg718/entropylab-esp32p4.git
cd entropylab-esp32p4
git fetch origin release/education-ui-20260917-docs147
git checkout --detach origin/release/education-ui-20260917-docs147
git rev-parse HEAD
```

Record the printed commit SHA in every bug report or test result. A branch push is not a merge to `main` and does not create a tag or downloadable firmware release.

## Set up and test

Follow [docs/BUILD-FLASH.md](docs/BUILD-FLASH.md). The intended full host command is:

```sh
rustup toolchain install 1.95.0 --profile minimal
export LVGL_SOURCE_DIR="$HOME/entropylab-deps/lvgl"
bash scripts/test-host.sh
```

Use LVGL commit `85aa60d18b3d5e5588d7b247abf90198f07c8a63`. All Cargo commands use lockfiles. Do not refresh a lockfile merely to make a failing test pass.

Current publication gap: a clean package run reaches many passing Rust/vector checks, then `scripts/test-host.sh` exits 1 because `verify-dice-import.py` still pins a historical `gui.c` identity. A separate clean `scripts/test-gui-host.sh` build completes and then exits 134 on a stale Safety/About caption assertion. Report these exact known failures; do not call the host gate green and do not rebaseline them incidentally. The accepted modal140 focused host binary separately passed its eleven scoped executions.

Target changes must also pass the pinned target build with ESP-IDF v5.5.5 commit `b774170ff46c393eeb5e495ea37936038d3f4f4f` and Rust `nightly-2026-04-15`. A target build is not permission to connect or flash hardware.
