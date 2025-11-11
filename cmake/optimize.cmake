# Extract GCC native optimization flags
if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "GCC optimization flags do not support Clang. Skipping.")
    return()
endif()

execute_process(
    COMMAND sh -c "gcc -### -E - -march=native -mtune=native -mcpu=native 2>&1 | sed -r '/cc1/!d; s/\"//g; s/^.* - //g; s/ -dumpbase.*//'"
    OUTPUT_VARIABLE OPTIMIZED_C_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(OPTIMIZED_C_FLAGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPTIMIZED_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPTIMIZED_C_FLAGS}")
    message(STATUS "Applied GCC optimization flags: ${OPTIMIZED_C_FLAGS}")
else()
    message(WARNING "Could not extract GCC optimization flags")
endif()
