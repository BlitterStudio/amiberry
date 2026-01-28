# Fix libserialport for Android builds.
#
# The scottmudge/libserialport-cmake fork generates a config.h where SP_API is
# set to __declspec(dllexport), which is a Windows-specific attribute.
# Clang (Android NDK) errors out unless -fdeclspec is enabled.
#
# We patch config.h to make SP_API empty on non-Windows platforms.

if(NOT DEFINED LIBSERIALPORT_SOURCE_DIR)
  message(FATAL_ERROR "LIBSERIALPORT_SOURCE_DIR not set")
endif()

set(cfg "${LIBSERIALPORT_SOURCE_DIR}/config.h")
if(NOT EXISTS "${cfg}")
  message(FATAL_ERROR "libserialport config.h not found at: ${cfg}")
endif()

file(READ "${cfg}" _c)

# Replace the problematic export definition.
# Keep it simple: on Android we build static, so we don't need dllexport.
string(REPLACE "#define SP_API __declspec(dllexport)" "#define SP_API" _c "${_c}")
string(REPLACE "#define SP_API __declspec(dllimport)" "#define SP_API" _c "${_c}")

file(WRITE "${cfg}" "${_c}")
message(STATUS "Patched libserialport config.h for Android: ${cfg}")
