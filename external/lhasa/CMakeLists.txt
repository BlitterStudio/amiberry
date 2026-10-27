# Lhasa (ISC licence), vendored at release v0.6.0 - see README.md.
# Static library target consumed by Amiberry; the upstream autotools build
# is not vendored.

set(LHASA_SOURCES
    lib/crc16.c
    lib/ext_header.c
    lib/lha_decoder.c
    lib/lha_endian.c
    lib/lha_file_header.c
    lib/lha_input_stream.c
    lib/lha_basic_reader.c
    lib/lha_reader.c
    lib/macbinary.c
    lib/null_decoder.c
    lib/lh1_decoder.c
    lib/lh5_decoder.c
    lib/lh6_decoder.c
    lib/lh7_decoder.c
    lib/lhx_decoder.c
    lib/lk7_decoder.c
    lib/lz5_decoder.c
    lib/lzs_decoder.c
    lib/pm1_decoder.c
    lib/pm2_decoder.c
)

if(WIN32)
    list(APPEND LHASA_SOURCES lib/lha_arch_win32.c)
else()
    list(APPEND LHASA_SOURCES lib/lha_arch_unix.c)
endif()

# bit_stream_reader.c, lh_new_decoder.c, pma_common.c and tree_decode.c are
# #included by the decoders above and must not be compiled separately.

add_library(lhasa STATIC ${LHASA_SOURCES})
target_include_directories(lhasa PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set_target_properties(lhasa PROPERTIES
    C_STANDARD 99
    POSITION_INDEPENDENT_CODE ON
)
