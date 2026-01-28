# Fix PortMidi sources for Android builds.
# This is intentionally deterministic and does not depend on patch context.
# Usage: cmake -DPORTMIDI_SOURCE_DIR=<dir> -P portmidi_fix_android.cmake

if(NOT DEFINED PORTMIDI_SOURCE_DIR)
  message(FATAL_ERROR "PORTMIDI_SOURCE_DIR not set")
endif()

set(pmutil "${PORTMIDI_SOURCE_DIR}/pm_common/pmutil.c")
if(NOT EXISTS "${pmutil}")
  message(FATAL_ERROR "PortMidi pmutil.c not found at: ${pmutil}")
endif()

file(READ "${pmutil}" _content)

# Replace the one remaining problematic bzero() usage with memset().
# (PortMidi uses bzero without including <strings.h> on non-WIN32 builds.)
string(REPLACE "bzero((char *) &queue->buffer[head], sizeof(int32_t) * queue->msg_size);"
               "memset((char *)&queue->buffer[head], 0, sizeof(int32_t) * queue->msg_size);"
               _content "${_content}")

# Also handle any other bzero(queue->buffer, ...) occurrences defensively.
string(REPLACE "bzero(queue->buffer, queue->len * sizeof(int32_t));"
               "memset(queue->buffer, 0, queue->len * sizeof(int32_t));"
               _content "${_content}")

file(WRITE "${pmutil}" "${_content}")
message(STATUS "Applied Android fixes to PortMidi: ${pmutil}")

if(NOT DEFINED AMIBERRY_SOURCE_DIR)
  message(FATAL_ERROR "AMIBERRY_SOURCE_DIR not set")
endif()

set(pm_common_cmake "${PORTMIDI_SOURCE_DIR}/pm_common/CMakeLists.txt")
if(EXISTS "${pm_common_cmake}")
  # Install an Android-safe PortTime backend into the PortMidi source tree.
  set(ptandroid_src "${PORTMIDI_SOURCE_DIR}/porttime/ptandroid.c")
  file(COPY_FILE "${AMIBERRY_SOURCE_DIR}/cmake/portmidi_ptandroid.c" "${ptandroid_src}")

  file(READ "${pm_common_cmake}" _pm_cmake)

  # On Android, PortMidi falls into the UNIX branch where it unconditionally adds ptlinux.c.
  # ptlinux.c includes sys/timeb.h which is missing in the NDK.
  # Replace ptlinux.c with our Android-ready implementation.
  string(REPLACE "${PMDIR}/porttime/ptlinux.c"
                 "${PMDIR}/porttime/ptandroid.c"
                 _pm_cmake "${_pm_cmake}")

  file(WRITE "${pm_common_cmake}" "${_pm_cmake}")
  message(STATUS "Patched PortMidi CMake for Android (ptlinux.c -> ptandroid.c): ${pm_common_cmake}")
else()
  message(WARNING "PortMidi pm_common/CMakeLists.txt not found; cannot patch PortTime backend")
endif()
