# Building

## Termux on Android

Run `sh scripts/termux_setup.sh`, then `APEX_JOBS=2 sh scripts/termux_build.sh`. This configures an incremental Ninja build under `build/termux`. The setup script installs packages without sudo. Availability and CMake package metadata for Qt 6 vary across Termux repositories; if Qt 6 Widgets cannot be found, the Windows Actions build remains the supported target build.

`sh scripts/build_windows.sh` prints the GitHub Actions workflow location. Termux itself is not treated as a Windows cross-compiler. The Windows workflow uses a Windows runner and packages the executable with Qt runtime DLLs.

## Linux

Install CMake, Ninja, a C++20 compiler, and Qt 6 Widgets. `sh scripts/run_tests.sh` currently performs a Debug configure/build only; there are no automated test cases yet.

## Cleaning

`sh scripts/clean.sh` removes generated `build/` and `package/` directories.
