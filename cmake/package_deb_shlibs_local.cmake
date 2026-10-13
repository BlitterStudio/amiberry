# Spell out "libfooNN | libfoo-extraNN" alternatives for FFmpeg libraries that
# Debian/Ubuntu ship in a plain and an -extra flavour. dpkg-shlibdeps would
# emit only "libavcodecNN (>= ...)"; apt satisfies that through the versioned
# Provides of libavcodec-extraNN, but python-apt based .deb installers (Linux
# Mint's Captain, GDebi) ignore versioned Provides and replace the installed
# -extra package instead (issue #2423).
#
# CPackDeb runs dpkg-shlibdeps with CPACK_TEMPORARY_DIRECTORY as its working
# directory, where dpkg-shlibdeps reads debian/shlibs.local in preference to
# package symbols files. CPackDeb removes that debian/ directory once the
# dependency list is generated, so the file never reaches the package payload.
if(NOT CPACK_GENERATOR STREQUAL "DEB" OR NOT CPACK_AMIBERRY_DEB_EXTRA_FLAVOUR_LIBS)
    return()
endif()

set(_amiberry_shlibs_local "")
foreach(_amiberry_lib IN LISTS CPACK_AMIBERRY_DEB_EXTRA_FLAVOUR_LIBS)
    # Entries are "<library>:<soversion>", e.g. "libavcodec:61".
    string(REPLACE ":" ";" _amiberry_lib "${_amiberry_lib}")
    list(GET _amiberry_lib 0 _amiberry_name)
    list(GET _amiberry_lib 1 _amiberry_soversion)

    # Lower bound: the upstream version of the -dev package built against,
    # matching what the symbols files yield for the other FFmpeg libraries.
    set(_amiberry_minver "")
    execute_process(
            COMMAND dpkg-query -W --showformat=\${Version} "${_amiberry_name}-dev"
            OUTPUT_VARIABLE _amiberry_devver
            RESULT_VARIABLE _amiberry_query_result
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_amiberry_query_result EQUAL 0 AND _amiberry_devver)
        string(REGEX REPLACE "-[^-]*$" "" _amiberry_devver "${_amiberry_devver}")
        set(_amiberry_minver " (>= ${_amiberry_devver})")
    endif()

    string(APPEND _amiberry_shlibs_local
            "${_amiberry_name} ${_amiberry_soversion} "
            "${_amiberry_name}${_amiberry_soversion}${_amiberry_minver} | "
            "${_amiberry_name}-extra${_amiberry_soversion}${_amiberry_minver}\n")
endforeach()

file(WRITE "${CPACK_TEMPORARY_DIRECTORY}/debian/shlibs.local" "${_amiberry_shlibs_local}")
