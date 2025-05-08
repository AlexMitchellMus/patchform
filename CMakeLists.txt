cmake_minimum_required(VERSION 3.30)
project(PlugPatch LANGUAGES CXX)

option(ENABLE_ASAN "Enable Address Sanitizer" ON)
option(RUN_TESTS "Build and run tests" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
if (ENABLE_ASAN AND MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL" CACHE STRING "" FORCE)
else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
endif()

# Define the Git command to get the full commit hash
set(BUILDINFO_CPP ${CMAKE_BINARY_DIR}/GitInfo.cpp)

# Read .git/HEAD to determine the current ref file.
file(READ "${CMAKE_SOURCE_DIR}/.git/HEAD" HEAD_CONTENTS)
string(REGEX MATCH "ref: (.*)" _ ${HEAD_CONTENTS})
if(CMAKE_MATCH_1)
    # Construct the full path to the ref file.
    set(GIT_REF_FILE "${CMAKE_SOURCE_DIR}/.git/${CMAKE_MATCH_1}")
    message(STATUS "Git ref file: ${GIT_REF_FILE}")
else()
    # If HEAD doesn't contain a ref (detached head), fallback to HEAD.
    set(GIT_REF_FILE "${CMAKE_SOURCE_DIR}/.git/HEAD")
endif()

set(GIT_VERSION_FILE "${CMAKE_BINARY_DIR}/GitVersion.txt")

add_custom_command(
        OUTPUT ${BUILDINFO_CPP}
        COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_FILE=${BUILDINFO_CPP}
        -DGIT_VERSION_FILE=${GIT_VERSION_FILE}
        -P ${CMAKE_SOURCE_DIR}/GenerateGitInfo.cmake
        DEPENDS ${CMAKE_SOURCE_DIR}/.git/HEAD ${GIT_REF_FILE}
        COMMENT "Generating buildinfo.cpp with updated Git info"
        VERBATIM
)

add_custom_target(update_git_version ALL DEPENDS ${BUILDINFO_CPP})

# Check GitVersion.txt if it exists
if (EXISTS "${CMAKE_BINARY_DIR}/GitVersion.txt")
    file(READ "${CMAKE_BINARY_DIR}/GitVersion.txt" PATCHFORM_VERSION_STRING)
    string(STRIP "${PATCHFORM_VERSION_STRING}" PATCHFORM_VERSION_STRING)
else()
    set(PATCHFORM_VERSION_STRING "dev")
endif()

message(STATUS "Detected Patchform version: ${PATCHFORM_VERSION_STRING}")

# Strip leading 'v' and extra stuff (e.g., "-alpha-17")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+).*" "\\1,\\2,\\3,0" PATCHFORM_VERSION_COMMA "${PATCHFORM_VERSION_STRING}")


add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/patchformicon.ico"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/installer/windows/patchformicon.ico"
        "${CMAKE_BINARY_DIR}/patchformicon.ico"
        DEPENDS "${CMAKE_SOURCE_DIR}/installer/windows/patchformicon.ico"
        COMMENT "Copying patchformicon.ico"
)

add_custom_target(copy_icon ALL DEPENDS "${CMAKE_BINARY_DIR}/patchformicon.ico")

set(PATCHFORM_ICON_PATH "${CMAKE_BINARY_DIR}/patchformicon.ico")

configure_file(
        ${CMAKE_SOURCE_DIR}/installer/windows/resources.rc.in
        ${CMAKE_BINARY_DIR}/version.rc
        @ONLY
)

# Toggle AVX2 for Glaze based on build type
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(glaze_ENABLE_AVX2 OFF CACHE BOOL "Disable AVX2 for Glaze in Debug mode" FORCE)
else()
    set(glaze_ENABLE_AVX2 ON CACHE BOOL "Enable AVX2 for Glaze in non-Debug modes" FORCE)
endif()

# External dependencies
set(PA_USE_ASIO ON CACHE BOOL "Enable ASIO support" FORCE)
add_subdirectory(external/portaudio)
add_subdirectory(external/json)
add_subdirectory(external/linenoise-ng)
add_subdirectory(external/unordered_dense)
add_subdirectory(external/glaze)

set(SDL_SHARED OFF CACHE BOOL "Disable SDL shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Enable SDL static library" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "Disable SDL audio support" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "Disable SDL joystick support" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "Disable SDL haptic support" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "Disable SDL sensor support" FORCE)
set(SDL_POWER OFF CACHE BOOL "Disable SDL power support" FORCE)
set(SDL_RENDER OFF CACHE BOOL "Disable SDL renderer support" FORCE)
set(SDL_RENDER_GPU OFF CACHE BOOL "Disable GPU rendering support" FORCE)
add_subdirectory(external/SDL2)
add_subdirectory(external/rtmidi)
add_subdirectory(external/libsamplerate)

add_subdirectory(Nodes/nodes/Metronome)

#==== nanovg static wrapper =====
# We use a wrapper to easily include nanovg without having to worry about ifdef/undef framebuffer stuff

add_library(NanoVGWrapper STATIC
        ${CMAKE_SOURCE_DIR}/Glad/NanoVGWrapper.cpp
        ${CMAKE_SOURCE_DIR}/external/nanovg/src/nanovg.cpp
)

set_target_properties(NanoVGWrapper PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
)

target_include_directories(NanoVGWrapper PUBLIC
        ${CMAKE_SOURCE_DIR}/external/nanovg/src
        ${CMAKE_SOURCE_DIR}/Glad/
)

target_link_libraries(NanoVGWrapper PRIVATE glad)

add_dependencies(NanoVGWrapper glad)

target_compile_definitions(NanoVGWrapper PRIVATE NANOVG_GL3_IMPLEMENTATION)

#==== nanovg static wrapper end =====

if (ENABLE_ASAN AND MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(nanovg PRIVATE /fsanitize=address)
    target_compile_options(rtmidi PRIVATE /fsanitize=address)
endif()

# Add moodycamel concurrentqueue
add_library(moodycamel_concurrentqueue INTERFACE)
target_include_directories(moodycamel_concurrentqueue INTERFACE "${CMAKE_SOURCE_DIR}/external/concurrentqueue")

# Add moodycamel readerwriterqueue
add_library(moodycamel_readerwriterqueue INTERFACE)
target_include_directories(moodycamel_readerwriterqueue INTERFACE "${CMAKE_SOURCE_DIR}/external/readerwriterqueue")

add_library(pffft STATIC external/pffft/pffft.c)
target_include_directories(pffft PUBLIC external/pffft)

target_compile_options(pffft PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Release>:/Ox /arch:AVX2 /fp:fast /Ob3 /GL /DNDEBUG>
        $<$<CONFIG:Debug>:/Od /Zi>
        >
)

add_library(glad STATIC Glad/glad.cpp)
target_include_directories(glad PUBLIC "${CMAKE_SOURCE_DIR}/Glad")

# Collect common source files
file(GLOB_RECURSE GRAPH_SOURCES "${CMAKE_SOURCE_DIR}/Graph/*.cpp" "${CMAKE_SOURCE_DIR}/Utility/*.cpp")

file(GLOB NODE_SHARED_SOURCES "${CMAKE_SOURCE_DIR}/Nodes/*.cpp")

# PlugPatchCLI target
add_executable(PlugPatchCLI PlugPatchCLI.cpp ${GRAPH_SOURCES})

target_include_directories(PlugPatchCLI PRIVATE
        "${CMAKE_SOURCE_DIR}/external/portaudio/include"
        "${CMAKE_SOURCE_DIR}/external/concurrentqueue"
        "${CMAKE_SOURCE_DIR}/external/json/single_include/nlohmann"
        "${CMAKE_SOURCE_DIR}/external/linenoise-ng/include"
        "${CMAKE_SOURCE_DIR}/external/unordered_dense/include/ankerl"
        "${CMAKE_SOURCE_DIR}/external/glaze/include/"
)
target_link_libraries(PlugPatchCLI PRIVATE
        linenoise
        moodycamel_concurrentqueue
        moodycamel_readerwriterqueue
        unordered_dense
        glaze::glaze
        portaudio
)
# Fix for linenoise not being updated to C++20
target_compile_options(linenoise PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:char8_t->             # Disable char8_t for MSVC
        $<$<CXX_COMPILER_ID:GNU>:-fno-char8_t>              # Disable char8_t for GCC
        $<$<CXX_COMPILER_ID:Clang>:-fno-char8_t>            # Disable char8_t for Clang
)

# Collect common source files
file(GLOB_RECURSE UI_SOURCES "${CMAKE_SOURCE_DIR}/UI/*.cpp" "${CMAKE_SOURCE_DIR}/UI_ToolKit/*.cpp")

# PlugPatchStandalone target - use WIN32 to hide terminal
add_executable(PatchformStandalone PlugPatchStandalone.cpp ${GRAPH_SOURCES} ${NODE_SHARED_SOURCES} ${UI_SOURCES} ${BUILDINFO_CPP})

set_target_properties(PatchformStandalone PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
)

target_compile_definitions(PatchformStandalone PRIVATE PATCHFORM_WITH_GUI NANOVG_GL3_IMPLEMENTATION=1)
target_include_directories(PatchformStandalone PRIVATE
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/external"
        "${CMAKE_SOURCE_DIR}/external/concurrentqueue"
        "${CMAKE_SOURCE_DIR}/external/readerwriterqueue"
        "${CMAKE_SOURCE_DIR}/external/json/single_include/nlohmann"
        "${CMAKE_SOURCE_DIR}/external/unordered_dense/include/ankerl"
        "${CMAKE_SOURCE_DIR}/external/glaze/include/"
        "${CMAKE_SOURCE_DIR}/external/portaudio/include"
        "${CMAKE_SOURCE_DIR}/Glad/"
        "${CMAKE_SOURCE_DIR}/external/nanovg/src/"
        "${CMAKE_SOURCE_DIR}/external/SDL2/include"
        "${CMAKE_SOURCE_DIR}/external/pffft"
        "${CMAKE_SOURCE_DIR}/external/rtmidi"
        "${CMAKE_SOURCE_DIR}/external/simde"
        "${CMAKE_SOURCE_DIR}/external/libsamplerate/include"
        "${CMAKE_SOURCE_DIR}/external/SignalsmithDSP/include"
)
target_link_libraries(PatchformStandalone PRIVATE
        SDL3::SDL3-static
        portaudio
        moodycamel_concurrentqueue
        moodycamel_readerwriterqueue
        unordered_dense
        glaze::glaze
        NanoVGWrapper
        pffft
        rtmidi
        glad
        samplerate
)

target_link_directories(PatchformStandalone PRIVATE
        "${CMAKE_BINARY_DIR}/bin/Debug"
)

target_compile_definitions(PatchformStandalone PRIVATE
        -DPA_USE_ASIO=ON
        SAMPLERATE_STATIC
)
if (MSVC)
    if (ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Building with MSVC ASAN")

        # Path to ASAN libs
        set(ASAN_LIB_PATH "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.43.34808/lib/x64")
        target_link_directories(PatchformStandalone PRIVATE "${ASAN_LIB_PATH}")

        target_compile_options(PatchformStandalone PRIVATE /fsanitize=address /Od /Zi /bigobj /Zc:__cplusplus)
        target_link_options(PatchformStandalone PRIVATE /INFERASANLIBS)

        # Force all static libs to match runtime (important!)
        set_target_properties(nanovg PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
        set_target_properties(rtmidi PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
        set_target_properties(portaudio PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
        set_target_properties(pffft PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
    else()
        target_compile_options(PatchformStandalone PRIVATE
                /std:c++latest
                /Zc:preprocessor
                $<$<CONFIG:Debug>:/Od /Zi /RTC1 /bigobj /Zc:__cplusplus>
                $<$<CONFIG:Release>:/Ox /arch:AVX2 /GL /GS- /Ob3 /fp:fast /bigobj /DNDEBUG>
                $<$<CONFIG:RelWithDebInfo>:/Ox /Z7 /GL /bigobj>
        )
        add_link_options($<$<CONFIG:Debug>:/DEBUG>)
    endif()
endif()

if (ENABLE_ASAN AND MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ASAN_DLL_PATH "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.43.34808/bin/Hostx64/x64/clang_rt.asan_dynamic-x86_64.dll")

    if (EXISTS "${ASAN_DLL_PATH}")
        add_custom_command(TARGET PatchformStandalone POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${ASAN_DLL_PATH}"
                "$<TARGET_FILE_DIR:PatchformStandalone>")
    else()
        message(WARNING "ASAN DLL not found: ${ASAN_DLL_PATH}")
    endif()
endif()

# ======================== TEST SETUP ========================

if(RUN_TESTS)
    message(STATUS "Building tests")
    add_subdirectory(external/catch2)
    include(Catch)
    include(CTest)

    add_executable(Tests Tests/test_main.cpp)

    target_include_directories(Tests PRIVATE
        "${CMAKE_SOURCE_DIR}/external/catch2"
    )
    target_link_libraries(Tests PRIVATE Catch2::Catch2)
    # Auto-discover Catch2 tests
    catch_discover_tests(Tests)
endif()

# ======================== RESOURCE SETUP ========================

if (WIN32)
    target_sources(PatchformStandalone PRIVATE ${CMAKE_BINARY_DIR}/version.rc)
endif ()

# files to copy
set(SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/Patches/graph.json"
        "${CMAKE_SOURCE_DIR}/Patches/graph1.json"
        "${CMAKE_SOURCE_DIR}/Patches/graph2.json"
        "${CMAKE_SOURCE_DIR}/Patches/graph3.json"
        "${CMAKE_SOURCE_DIR}/Patches/graph4.json5"
        "${CMAKE_SOURCE_DIR}/Patches/count.json5"
        "${CMAKE_SOURCE_DIR}/Patches/WavetableMovement.json"
        "${CMAKE_SOURCE_DIR}/Patches/Inter-VariableFont_opsz,wght.ttf"
        "${CMAKE_SOURCE_DIR}/Assets/Icons/IconFontPlugPatch.ttf"
        "${CMAKE_SOURCE_DIR}/Assets/Icons/IconFontPlugPatch_google.ttf"
)

# Copy compiled object's into Patchform/Objects directory
add_custom_target(copy_objects ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_BINARY_DIR}/objects"
        "$<TARGET_FILE_DIR:PatchformStandalone>/Objects"
        COMMENT "Copying plugin objects to output directory"
)
# Make sure copy_objects runs AFTER metronome object is built!
add_dependencies(copy_objects Metronome)

# Define a custom target to copy the Patches directory
add_custom_target(copy_patches
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Patches"
        "$<TARGET_FILE_DIR:PatchformStandalone>/Patches"
        COMMENT "Copying Patches directory to the output directory"
)

# FIXME: Do not copy the whole icon dir, but just for now to test
add_custom_target(copy_fonts
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:PatchformStandalone>/Assets"
        COMMENT "Copying Fonts directory to the output directory"
)

# Add dependencies to ensure the directory is copied whenever these targets are built
add_dependencies(PlugPatchCLI copy_patches)
add_dependencies(PatchformStandalone
    copy_patches
    copy_fonts
    update_git_version
    copy_icon
    NanoVGWrapper

    # Plugin system
    #Metronome
    #copy_objects
    )

# Installer configuration for CLI
#TODO NOT WORKING ONLY FOR RELEASE for some reason! :-(
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(INSTALLER_DIR ${CMAKE_SOURCE_DIR}/installer)

    # -------------------------- CLI --------------------------
    set(NSIS_SCRIPT ${INSTALLER_DIR}/windows/installer.nsi)

    # Define the location of the built application
    set(APPLICATION_BINARY_CLI "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/PlugPatchCLI.exe")

    add_custom_target(PackageInstaller
            COMMAND ${CMAKE_COMMAND} -E echo "Creating NSIS installer..."
            COMMAND makensis.exe /DOUTPUT_DIR=$<TARGET_FILE_DIR:PlugPatchCLI> /DAPPLICATION_BINARY=${APPLICATION_BINARY_CLI} ${NSIS_SCRIPT}
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            COMMENT "Packaging installer with NSIS"
            VERBATIM
    )
    add_dependencies(PackageInstaller PlugPatchCLI)

    # -------------------------- GUI --------------------------

    set(NSIS_SCRIPT ${INSTALLER_DIR}/windows/installerStandalone.nsi)

    # Define the location of the built application
    set(APPLICATION_BINARY_STANDALONE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/PatchformStandalone.exe")

    add_custom_target(PackageInstallerStandalone
            COMMAND ${CMAKE_COMMAND} -E echo "Creating NSIS installer..."
            COMMAND makensis.exe
                /DOUTPUT_DIR=$<TARGET_FILE_DIR:PatchformStandalone>
                /DAPPLICATION_BINARY=${APPLICATION_BINARY_STANDALONE}
                /DAPP_VERSION=${PATCHFORM_VERSION_STRING}
                ${NSIS_SCRIPT}
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            COMMENT "Packaging installer with NSIS"
            VERBATIM
    )
    add_dependencies(PackageInstallerStandalone PatchformStandalone)
else()
    message(WARNING "The PackageInstallers target is only available in Release mode.")
endif()
