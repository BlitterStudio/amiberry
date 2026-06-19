---
name: amiberry-packaging-install
description: >
  Reference workflow for Amiberry packaging, install-layout, and runtime-path fixes. Use for
  distro packaging changes, `BUNDLE_SDL`, RPATH and shared-library bundling work, DEB or RPM
  dependency adjustments, CI matrix packaging toggles, derivative distro repository setup via
  `packaging/repo/install.sh`, QEMU-UAE PPC plugin bundle/dependency changes, Windows portable
  install detection, and executable-directory vs current-working-directory path bugs in files such as
  `cmake/linux/install.cmake`, `cmake/windows/install.cmake`, `packaging/CMakeLists.txt`,
  `packaging/repo/install.sh`, `.github/workflows/c-cpp.yml`, `docs/ppc-qemu-plugin.md`, and
  `src/osdep/amiberry.cpp`.
---

# Amiberry Packaging And Install Layout Reference

## Scope

- Use this skill for packaging and install layout work, not emulator-core fixes.
- Focus areas:
  - Linux packages and install-time library layout
  - Windows portable install, llvm-mingw runtime DLL bundling, and executable-relative path resolution
  - CI packaging toggles that change shipped dependencies
  - APT repository installer distro-family and suite mapping
  - QEMU-UAE PPC plugin packaging, artifact selection, and load-time dependency checks

## Linux Packaging Rules

- `BUNDLE_SDL=ON` means the package owns the SDL3 and SDL3_image runtime dependency story.
- If shared libraries are bundled into `${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}`, package metadata must stop declaring system SDL3 dependencies for that build variant.
- Install both the resolved shared library files and any required SONAME companions that the runtime linker will actually look up.
- If the binary expects bundled libraries, set `INSTALL_RPATH` to the installed package location and verify it matches the final layout.
- Keep distro-specific CI toggles aligned with repository availability. If Bookworm lacks SDL3 packages, the Bookworm matrix should not rely on system SDL3.

## Windows Portable Install Rules

- Never assume the current working directory is the executable directory.
- Portable markers such as `amiberry.portable` must be checked next to `amiberry.exe`, not only in the process working directory.
- Reuse one executable-directory helper for config, home, plugins, and data path decisions so launch-context differences do not split behavior.
- Windows packages must bundle the llvm-mingw runtime DLLs (`libc++.dll`, `libunwind.dll`, `libwinpthread-1.dll`) from `LLVM_MINGW_ROOT/bin`; vcpkg does not own these.
- Validate both portable and non-portable mode startup paths after changes.

## Repository Installer Rules

- Treat `/etc/os-release` as a compatibility signal, not a direct package-suite truth.
- Keep `APT_FAMILY` separate from `APT_SUITE`:
  - `APT_FAMILY` controls repository format decisions such as DEB822 cutoff rules.
  - `APT_SUITE` controls the suite written into the `.list` or `.sources` file.
- Prefer explicit derivative mappings before generic `ID_LIKE` fallback. Known examples:
  - LMDE, Devuan, Parrot, and Sparky map onto Debian suites.
  - Linux Mint, Elementary OS, Pop!_OS, and Zorin normally map through `UBUNTU_CODENAME`.
- Some distros expose misleading `ID`/`VERSION_CODENAME` pairs. For Parrot, inspect `NAME` or `PRETTY_NAME` as well as `ID` before falling back.
- When adding a derivative, update the unsupported-distribution message so users can see it is intentionally covered.

## QEMU-UAE PPC Plugin Packaging Rules

- The package must ship the plugin and verify that it can be loaded, not just that the file exists.
- Linux packages install `qemu-uae.so` under the plugin library directory and declare the system packages needed to load it.
- macOS bundles put `qemu-uae.dylib` under `Contents/Resources/plugins` and dependent dylibs under `Contents/Frameworks`.
- Windows packages include `plugins/qemu-uae.dll`.
- Official Windows QEMU-UAE plugin release artifacts are statically linked for non-system plugin dependencies such as GLib, GModule, and zlib. Do not require those plugin DLLs beside `Amiberry.exe` unless the selected artifact is known to be dynamically linked.
- Keep `QEMU_UAE_RELEASE_TAG` aligned across the shell downloader, PowerShell downloader, workflow environment, and documentation.

## Change Workflow

1. Identify whether the bug is packaging metadata, install layout, or runtime path resolution.
2. Trace the full chain:
   - CMake option
   - install rules
   - package dependency metadata
   - CI build variant
   - runtime lookup path
3. Update every layer that changed ownership of the dependency or path.
4. Verify the installed layout, not just the build-tree layout.

## Common Review Traps

- Bundling a shared library but leaving the old DEB or RPM dependency in place.
- Installing the real `.so` but not the SONAME path needed by the loader.
- Setting RPATH for the build tree instead of the final installed package location.
- Detecting portable mode from the working directory only, which breaks shell shortcuts and launcher-based starts.
- Fixing `get_config_directory()` without making `get_home_directory()`, `get_data_directory()`, or `locate_amiberry_conf()` consistent.
- Mapping a derivative distro directly to its displayed codename when the repository only has the parent Debian/Ubuntu suite.
- Checking only plugin file presence in CI while missing load-time dependency resolution.
- Expecting Windows QEMU-UAE plugin runtime DLLs after switching to statically linked plugin artifacts.

## Verification

- For Linux packaging changes:
  - inspect install rules
  - inspect package dependency lists
  - inspect CI matrix flags
  - test the installed package layout, not only `cmake --build`
- For Windows portable changes:
  - launch from the executable directory
  - launch from a different working directory
  - verify `amiberry.portable` next to the executable changes config/home resolution as intended
  - inspect the installed/package directory for the llvm-mingw runtime DLLs
- For repository installer changes:
  - test representative `/etc/os-release` combinations for direct and derivative distros
  - verify Debian/Ubuntu family selection separately from suite name
  - verify whether DEB822 or legacy `.list` output is selected from the mapped parent version
- For QEMU-UAE plugin changes:
  - verify plugin file presence in the final package
  - run a loader check or equivalent dependency-resolution check
  - verify Windows packages do not require obsolete plugin runtime DLLs when using static plugin artifacts
