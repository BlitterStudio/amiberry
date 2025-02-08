if(DEFINED TOOLCHAIN_ALREADY_RUN)
    return()
endif()
# Set the guard variable, because for some strange reason it's run twice if not set.
set(TOOLCHAIN_ALREADY_RUN TRUE CACHE INTERNAL "Toolchain file has already been processed")

if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "This toolchain file does not support Clang. Please use GCC or another supported compiler.")
endif()

# Check if cross-compiling is being attempted
if(CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "This toolchain file cannot be used for cross-compilation.")
endif()


# Run gcc to get the compilation details and remove unwanted flags after -dumpbase
execute_process(
        COMMAND sh -c "gcc -### -E - -march=native -mtune=native -mcpu=native 2>&1 | sed -r '/cc1/!d; s/\"//g; s/^.* - //g; s/ -dumpbase.*//'"
        OUTPUT_VARIABLE OPTIMIZED_C_FLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
