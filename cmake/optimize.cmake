# Check for cross-compilation
if(CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "WITH_OPTIMIZE cannot be used with cross-compilation. Native optimizations require host CPU detection.")
endif()

# Extract compiler-specific optimization flags
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    execute_process(
        COMMAND sh -c "clang -mcpu=native -### -c -x c /dev/null 2>&1 | grep -o '\"-target-cpu\" \"[^\"]*\"' | cut -d'\"' -f4"
        OUTPUT_VARIABLE CLANG_TARGET_CPU
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(CLANG_TARGET_CPU)
        set(OPTIMIZED_FLAGS "-mcpu=${CLANG_TARGET_CPU}")
        message(STATUS "Applied Clang optimization flags: ${OPTIMIZED_FLAGS}")
    else()
        message(WARNING "Could not extract Clang optimization flags")
    endif()
else()
    execute_process(
        COMMAND sh -c "gcc -### -E - -march=native -mtune=native -mcpu=native 2>&1 | sed -r '/cc1/!d; s/\"//g; s/^.* - //g; s/ -dumpbase.*//'"
        OUTPUT_VARIABLE OPTIMIZED_FLAGS_STRING
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(OPTIMIZED_FLAGS_STRING)
        # Convert space-separated string to list and filter out invalid options
        string(REPLACE " " ";" OPTIMIZED_FLAGS_LIST "${OPTIMIZED_FLAGS_STRING}")
        set(OPTIMIZED_FLAGS "")
        foreach(flag ${OPTIMIZED_FLAGS_LIST})
            if(NOT flag MATCHES "^--param" AND NOT flag MATCHES "^[^-]" AND NOT flag MATCHES "cache.*=")
                list(APPEND OPTIMIZED_FLAGS "${flag}")
            endif()
        endforeach()
        message(STATUS "Applied compiler optimization flags: ${OPTIMIZED_FLAGS}")
    else()
        message(WARNING "Could not extract GCC optimization flags")
    endif()
endif()

# Performance and memory optimization flags
set(PERF_FLAGS "-ffast-math" "-funroll-loops" "-finline-functions" "-fomit-frame-pointer")
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    list(APPEND PERF_FLAGS "-fprefetch-loop-arrays" "-finline-limit=600")
endif()

add_compile_options(${OPTIMIZED_FLAGS} ${PERF_FLAGS})


