# Amiberry Android - short FetchContent/object paths for Windows hosts
#
# Loaded at the end of the top-level project() through
# CMAKE_PROJECT_amiberry_INCLUDE (wired in android/app/build.gradle), so it
# runs before cmake/Dependencies.cmake invokes FetchContent.
#
# Android Studio/AGP keeps the main CMake build tree deeply nested below
# android/app/.cxx. SDL3_image then creates additional nested codec build
# trees (libwebp, libjxl, libtiff, ...). On Windows, Ninja can fail with the
# misleading message "mkdir(...): No such file or directory" when an object
# path becomes too long.
#
# Two mitigations:
#   1. FetchContent is relocated to a very short directory at the root of the
#      SAME drive as the Amiberry source tree.
#   2. CMAKE_OBJECT_PATH_MAX is reduced so CMake hashes long object paths
#      sooner.
#
# ABI and build type remain separated. A short hash of CMAKE_SOURCE_DIR
# prevents collisions if multiple Amiberry source trees exist on the same
# drive.

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    # Git for Windows keeps long-path support disabled by default. SDL3_image
    # contains recursively nested codec submodules (notably libjxl/LittleCMS).
    # Apply core.longpaths only to Git processes spawned by this CMake
    # configure; this does NOT modify the user's global/system Git
    # configuration.
    if(CMAKE_HOST_WIN32)
        set(ENV{GIT_CONFIG_COUNT} "1")
        set(ENV{GIT_CONFIG_KEY_0} "core.longpaths")
        set(ENV{GIT_CONFIG_VALUE_0} "true")
        message(STATUS "Amiberry Android Git long-path support: enabled for FetchContent")
    endif()

    if(DEFINED ANDROID_ABI AND NOT ANDROID_ABI STREQUAL "")
        set(_AMIBERRY_ANDROID_ABI "${ANDROID_ABI}")
    else()
        set(_AMIBERRY_ANDROID_ABI "unknown-abi")
    endif()

    if(DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL "")
        set(_AMIBERRY_ANDROID_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
    else()
        set(_AMIBERRY_ANDROID_BUILD_TYPE "Default")
    endif()

    # Obtain the Windows drive root (for example "D:") from the source path.
    set(_AMIBERRY_SOURCE_PATH "${CMAKE_SOURCE_DIR}")
    cmake_path(GET _AMIBERRY_SOURCE_PATH ROOT_NAME _AMIBERRY_ROOT_NAME)

    # Keep different source checkouts isolated without using their long names.
    string(SHA1 _AMIBERRY_SOURCE_HASH_FULL "${CMAKE_SOURCE_DIR}")
    string(SUBSTRING "${_AMIBERRY_SOURCE_HASH_FULL}" 0 8 _AMIBERRY_SOURCE_HASH)

    if(CMAKE_HOST_WIN32 AND NOT _AMIBERRY_ROOT_NAME STREQUAL "")
        set(_AMIBERRY_FETCHCONTENT_ROOT
            "${_AMIBERRY_ROOT_NAME}/.amiberry-fc/${_AMIBERRY_SOURCE_HASH}")
    else()
        # Non-Windows hosts (and drive-less Windows paths) fall back to a
        # hidden directory next to the sources; path length is not a problem
        # there.
        set(_AMIBERRY_FETCHCONTENT_ROOT
            "${CMAKE_SOURCE_DIR}/.fc/${_AMIBERRY_SOURCE_HASH}")
    endif()

    file(MAKE_DIRECTORY "${_AMIBERRY_FETCHCONTENT_ROOT}")

    set(
        _AMIBERRY_FETCHCONTENT_BASE
        "${_AMIBERRY_FETCHCONTENT_ROOT}/${_AMIBERRY_ANDROID_ABI}-${_AMIBERRY_ANDROID_BUILD_TYPE}"
    )

    set(
        FETCHCONTENT_BASE_DIR
        "${_AMIBERRY_FETCHCONTENT_BASE}"
        CACHE PATH
        "Short FetchContent directory for Amiberry Android builds"
        FORCE
    )

    # CMake normally hashes object paths only after reaching its platform
    # limit. Ninja/Win32 can fail earlier in deeply nested third-party
    # projects. 180 is safely above CMake's documented minimum of 128 while
    # forcing earlier hashing for libwebp/libjxl object trees.
    set(
        CMAKE_OBJECT_PATH_MAX
        "180"
        CACHE STRING
        "Maximum object file path length for Windows Android/Ninja builds"
        FORCE
    )

    message(
        STATUS
        "Amiberry Android FetchContent base: ${FETCHCONTENT_BASE_DIR}"
    )
    message(
        STATUS
        "Amiberry Android CMAKE_OBJECT_PATH_MAX: ${CMAKE_OBJECT_PATH_MAX}"
    )

    unset(_AMIBERRY_FETCHCONTENT_BASE)
    unset(_AMIBERRY_FETCHCONTENT_ROOT)
    unset(_AMIBERRY_SOURCE_HASH)
    unset(_AMIBERRY_SOURCE_HASH_FULL)
    unset(_AMIBERRY_ROOT_NAME)
    unset(_AMIBERRY_SOURCE_PATH)
    unset(_AMIBERRY_ANDROID_ABI)
    unset(_AMIBERRY_ANDROID_BUILD_TYPE)
endif()
