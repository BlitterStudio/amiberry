# Check for cross-compilation
if(CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "WITH_OPTIMIZE cannot be used with cross-compilation. Native optimizations require host CPU detection.")
endif()

# Sets <out> to the command that runs the compiler CMake uses for <lang>,
# without launchers given with it in CC/CXX but with any flags given there
# (with CC="ccache distcc clang -m32", CMAKE_C_COMPILER is ccache and
# "distcc clang -m32" is ARG1, so this gives "clang;-m32").
function(get_probe_command lang out)
    set(words "${CMAKE_${lang}_COMPILER}")
    if(CMAKE_${lang}_COMPILER_ARG1)
        separate_arguments(arg1 UNIX_COMMAND "${CMAKE_${lang}_COMPILER_ARG1}")
        list(APPEND words ${arg1})
    endif()
    set(command "")
    foreach(word IN LISTS words)
        if(word MATCHES "^-")
            list(APPEND command "${word}")
        else()
            set(command "${word}")
        endif()
    endforeach()
    set(${out} "${command}" PARENT_SCOPE)
endfunction()

# Runs the clang driver with the given flags and sets CLANG_RESULT (its exit
# code) and CLANG_OUTPUT (the jobs it would run).
function(query_clang_driver)
    execute_process(
        COMMAND ${PROBE_COMMAND} ${ARGN} "-###" -c -x c /dev/null
        RESULT_VARIABLE result
        ERROR_VARIABLE output
        OUTPUT_QUIET
    )
    set(CLANG_RESULT "${result}" PARENT_SCOPE)
    set(CLANG_OUTPUT "${output}" PARENT_SCOPE)
endfunction()

# Sets CLANG_FEATURES to the target features (+name/-name) clang compiles a
# function with for the given flags. These are only complete in the IR: for
# x86 the driver passes just the CPU name and cc1 expands its features.
function(query_clang_features)
    set(source "${CMAKE_BINARY_DIR}/CMakeFiles/optimize_probe.c")
    if(NOT EXISTS "${source}")
        file(WRITE "${source}" "void f(void) {}\n")
    endif()
    execute_process(
        COMMAND ${PROBE_COMMAND} ${ARGN} -S -emit-llvm -o - "${source}"
        OUTPUT_VARIABLE ir
        ERROR_QUIET
    )
    set(features "")
    if(ir MATCHES "\"target-features\"=\"([^\"]*)\"")
        string(REPLACE "," ";" features "${CMAKE_MATCH_1}")
    endif()
    set(CLANG_FEATURES "${features}" PARENT_SCOPE)
endfunction()

# Sets <out> to the features in <features> that are enabled (+name), without the +.
function(enabled_features features out)
    set(enabled "")
    foreach(feature IN LISTS features)
        if(feature MATCHES "^\\+(.+)")
            list(APPEND enabled "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    set(${out} "${enabled}" PARENT_SCOPE)
endfunction()

# Sets <out> to explicit flags for what native means for <lang>'s compiler on
# this host, so they can be passed on (distcc compiles anything with
# -march=native locally).
function(detect_native_flags lang out)
    get_probe_command(${lang} PROBE_COMMAND)
    set(flags "")

    if(CMAKE_${lang}_COMPILER_ID MATCHES "Clang")
        # On AArch64 only -mcpu=native reliably detects the host's features:
        # before clang 23, -march=native gives just the base architecture's
        # defaults and overrides the features from -mcpu. x86 rejects -mcpu
        # (-### still prints a job with the default CPU) and takes the CPU
        # name with -march, which also tunes for it.
        query_clang_driver(-mcpu=native)
        set(cpu_option "-mcpu")
        if(CLANG_OUTPUT MATCHES "unsupported option '-mcpu='")
            set(cpu_option "-march")
            query_clang_driver(-march=native)
        endif()
        if(CLANG_RESULT EQUAL 0 AND CLANG_OUTPUT MATCHES "\"-target-cpu\" \"([^\"]+)\"")
            set(cpu "${CMAKE_MATCH_1}")
            # A generic x86 CPU means detection failed
            if(NOT (cpu_option STREQUAL "-march" AND cpu MATCHES "^(x86-64|i[3-6]86)$"))
                set(flags "${cpu_option}=${cpu}")

                # The CPU name alone gives LLVM's default features for it,
                # which this chip may not match (no crypto on a Pi 4, features
                # disabled by a VM, a core LLVM doesn't know coming back as
                # generic). Pass on the difference to the features clang chose
                # for native, in its own names. SHELL: keeps CMake from
                # de-duplicating the repeated -Xclang.
                query_clang_features(${cpu_option}=native)
                enabled_features("${CLANG_FEATURES}" native_enabled)
                query_clang_features(${flags})
                enabled_features("${CLANG_FEATURES}" cpu_enabled)
                if(NOT native_enabled)
                    message(FATAL_ERROR "WITH_OPTIMIZE could not get the target features of ${cpu_option}=native.")
                endif()
                # On x86 the CPU name enables all its features, and native
                # only adds ones x86-64 implies anyway (64bit, cmov), so only
                # what native lacks is passed on.
                if(cpu_option STREQUAL "-mcpu")
                    foreach(feature IN LISTS native_enabled)
                        if(NOT feature IN_LIST cpu_enabled)
                            list(APPEND flags "SHELL:-Xclang -target-feature -Xclang +${feature}")
                        endif()
                    endforeach()
                endif()
                foreach(feature IN LISTS cpu_enabled)
                    if(NOT feature IN_LIST native_enabled)
                        list(APPEND flags "SHELL:-Xclang -target-feature -Xclang -${feature}")
                    endif()
                endforeach()
            endif()
        endif()
    else()
        # -mcpu=native is needed on AArch64: GCC 14+ ignores -mtune=native
        # there, and GCC 13 to 16.2 drop -march=native for known cores when
        # -mtune or -mcpu is also given (PR124629). x86 treats -mcpu as a
        # deprecated -mtune. Joining --param with its value keeps CMake from
        # de-duplicating the repeated --param options.
        list(JOIN PROBE_COMMAND "' '" probe)
        execute_process(
            COMMAND sh -c "'${probe}' -### -E - -march=native -mtune=native -mcpu=native < /dev/null 2>&1 | sed -r '/cc1/!d; s/\"//g; s/^.* - //g; s/ -dumpbase.*//; s/--param /--param=/g'"
            OUTPUT_VARIABLE flags_string
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Keep the target options and tuning parameters, not other defaults
        # the distro's GCC passes to cc1 (e.g. -fasynchronous-unwind-tables)
        string(REPLACE " " ";" flags_list "${flags_string}")
        foreach(flag IN LISTS flags_list)
            if(flag MATCHES "^(-m|--param=)")
                list(APPEND flags "${flag}")
            endif()
        endforeach()

        # When GCC reports both, -march is the detected ISA and -mcpu only adds
        # the core to tune for (passing both makes GCC warn if they disagree).
        # Cores GCC doesn't know come back as generic-*, which -march already
        # tunes for.
        if(flags MATCHES "(^|;)-march=")
            foreach(flag IN LISTS flags)
                if(flag MATCHES "^-mcpu=([^+]+)")
                    set(cpu "${CMAKE_MATCH_1}")
                    list(REMOVE_ITEM flags "${flag}")
                    if(NOT cpu MATCHES "^generic" AND NOT flags MATCHES "(^|;)-mtune=")
                        list(APPEND flags "-mtune=${cpu}")
                    endif()
                endif()
            endforeach()
        endif()
    endif()

    if(NOT flags MATCHES "-m(arch|cpu)=")
        list(JOIN PROBE_COMMAND " " probe)
        message(FATAL_ERROR "WITH_OPTIMIZE could not extract optimization flags from ${probe} (${CMAKE_${lang}_COMPILER_ID}).")
    endif()
    set(${out} "${flags}" PARENT_SCOPE)
endfunction()

# Performance and memory optimization flags. Not -ffast-math: it makes isnan()
# and isinf() always false, which breaks the FPU emulation. -fno-math-errno is
# safe, as nothing checks errno after math functions, and lets e.g. sqrt() be
# inlined.
foreach(lang C CXX)
    detect_native_flags(${lang} ${lang}_FLAGS)
    list(APPEND ${lang}_FLAGS "-fno-math-errno" "-funroll-loops" "-finline-functions" "-fomit-frame-pointer")
    if(CMAKE_${lang}_COMPILER_ID MATCHES "GNU")
        list(APPEND ${lang}_FLAGS "-fprefetch-loop-arrays" "-finline-limit=600")
    endif()
endforeach()

# Applied globally, so bundled libraries are built for this CPU too
if(C_FLAGS STREQUAL CXX_FLAGS)
    add_compile_options(${C_FLAGS})
    list(JOIN C_FLAGS " " APPLIED_FLAGS)
    # Show clang's -Xclang -target-feature -Xclang +x/-x as just +x/-x
    string(REPLACE "SHELL:-Xclang -target-feature -Xclang " "" APPLIED_FLAGS "${APPLIED_FLAGS}")
    message(STATUS "Applied compiler optimization flags: ${APPLIED_FLAGS}")
else()
    # CC and CXX are different compilers or versions
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:C>:${C_FLAGS}>"
        "$<$<COMPILE_LANGUAGE:CXX,OBJCXX>:${CXX_FLAGS}>"
    )
    foreach(lang C CXX)
        list(JOIN ${lang}_FLAGS " " APPLIED_FLAGS)
        string(REPLACE "SHELL:-Xclang -target-feature -Xclang " "" APPLIED_FLAGS "${APPLIED_FLAGS}")
        message(STATUS "Applied ${lang} compiler optimization flags: ${APPLIED_FLAGS}")
    endforeach()
endif()
