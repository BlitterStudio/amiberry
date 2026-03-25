---
name: amiberry-packaging-install
description: >
  Reference workflow for Amiberry packaging, install-layout, and runtime-path fixes. Use for
  distro packaging changes, `BUNDLE_SDL`, RPATH and shared-library bundling work, DEB or RPM
  dependency adjustments, CI matrix packaging toggles, Windows portable install detection, and
  executable-directory vs current-working-directory path bugs in files such as
  `cmake/linux/install.cmake`, `packaging/CMakeLists.txt`, `.github/workflows/c-cpp.yml`, and
  `src/osdep/amiberry.cpp`.
---

# Amiberry Packaging And Install Layout Reference

## Scope

- Use this skill for packaging and install layout work, not emulator-core fixes.
- Focus areas:
  - Linux packages and install-time library layout
  - Windows portable install and executable-relative path resolution
  - CI packaging toggles that change shipped dependencies

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
- Validate both portable and non-portable mode startup paths after changes.

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
