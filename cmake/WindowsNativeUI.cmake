# Optional WinUI 3 frontend. The emulator stays llvm-mingw C++; .NET builds
# the unpackaged native UI as a separate, self-contained Windows process.
if(NOT WIN32)
    message(FATAL_ERROR "The native Windows UI requires Windows")
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(_amiberry_windows_ui_platform arm64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    set(_amiberry_windows_ui_platform x64)
else()
    message(FATAL_ERROR "The native Windows UI supports x64 and ARM64 only")
endif()

find_program(AMIBERRY_DOTNET_EXECUTABLE NAMES dotnet REQUIRED NO_CMAKE_FIND_ROOT_PATH)
execute_process(
    COMMAND "${AMIBERRY_DOTNET_EXECUTABLE}" --version
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE _amiberry_dotnet_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _amiberry_dotnet_result
)
if(NOT _amiberry_dotnet_result EQUAL 0 OR _amiberry_dotnet_version VERSION_LESS 10.0)
    message(FATAL_ERROR "The native Windows UI needs .NET SDK 10 or newer; install it or configure with AMIBERRY_NATIVE_WINDOWS_UI=OFF")
endif()

target_sources(${PROJECT_NAME} PRIVATE
    src/osdep/windows/native_host.cpp
    src/osdep/windows/settings_bridge.cpp
)
target_compile_definitions(${PROJECT_NAME} PRIVATE AMIBERRY_NATIVE_WINDOWS_UI)
target_link_libraries(${PROJECT_NAME} PRIVATE ole32 uuid comdlg32 comctl32 shell32)

# Let MSBuild track C#/XAML inputs and NuGet restore incrementally, including
# newly added pages. Each CMake build tree has its own intermediate directory.
add_custom_target(amiberry_windows_native_ui
    COMMAND "${AMIBERRY_DOTNET_EXECUTABLE}" publish
        "${CMAKE_SOURCE_DIR}/windows/Amiberry.Windows/Amiberry.Windows.csproj"
        --configuration "$<IF:$<CONFIG:Debug>,Debug,Release>"
        --runtime "win-${_amiberry_windows_ui_platform}"
        --self-contained true
        --output "$<TARGET_FILE_DIR:${PROJECT_NAME}>/native-ui"
        "-p:Platform=${_amiberry_windows_ui_platform}"
        "-p:WindowsAppSDKSelfContained=true"
        "-p:BaseIntermediateOutputPath=${CMAKE_CURRENT_BINARY_DIR}/windows-native-obj/"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Publishing the self-contained Windows native UI (${_amiberry_windows_ui_platform})"
    VERBATIM
    USES_TERMINAL
)
add_dependencies(${PROJECT_NAME} amiberry_windows_native_ui)
