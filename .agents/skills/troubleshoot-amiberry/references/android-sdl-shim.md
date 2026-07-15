# Android SDL Java/JNI Shim Synchronization

Android builds native SDL from the tag pinned in `cmake/Dependencies.cmake`, while Amiberry checks SDL's Java glue into `android/app/src/main/java/org/libsdl/app/`. SDL validates these halves at runtime. If their versions differ, launching the emulator GUI reports `SDL C/Java version mismatch`.

## Updating SDL

1. Read the old and new `sdl3` `GIT_TAG` values in `cmake/Dependencies.cmake`.
2. Obtain both upstream SDL tags with the authenticated `gh` CLI.
3. Diff `android-project/app/src/main/java/org/libsdl/app` between those tags. Inspect version constants, JNI signatures, and behavioral fixes.
4. Review every upstream Java delta and port the applicable changes into Amiberry's checked-in sources in the same change as the native tag bump. Version constants and JNI contract changes are mandatory.
5. Preserve Amiberry-specific Java changes, especially physical mouse transitions, pen device types, Android Back handling, test visibility, and other input fixes. Do not replace the whole directory blindly.
6. Update `SDLActivity`'s `SDL_MAJOR_VERSION`, `SDL_MINOR_VERSION`, and `SDL_MICRO_VERSION` to match the fetched tag.
7. Update `SDLJavaShimSignatureTest` for any JNI contract changes. Keep its version assertion derived from the `sdl3` tag in `Dependencies.cmake`; do not hard-code the expected SDL version in the test.

Do not trust an existing `android/app/.cxx/**/_deps/sdl3-src` checkout until Gradle/CMake has reconfigured it. Those directories can retain a previous FetchContent tag.

## Verification

Run the focused Java/JNI contract test, then build the complete APK for both configured ABIs:

```bash
cd android
./gradlew :app:testDebugUnitTest --tests com.blitterstudio.amiberry.ui.SDLJavaShimSignatureTest
./gradlew :app:assembleDebug
```

On an Android device, launch **Advanced Options** and confirm that SDL starts without a C/Java version mismatch. Treat the native SDL tag and checked-in Java glue as one atomic dependency update.
