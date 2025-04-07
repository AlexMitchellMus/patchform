cmake_minimum_required(VERSION 3.30)
project(PlugPatch LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(RUN_TESTS "Build and run tests" ON)

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

add_custom_command(
        OUTPUT ${BUILDINFO_CPP}
        COMMAND ${CMAKE_COMMAND} -DOUTPUT_FILE=${BUILDINFO_CPP} -P ${CMAKE_SOURCE_DIR}/GenerateGitInfo.cmake
        DEPENDS ${CMAKE_SOURCE_DIR}/.git/HEAD ${GIT_REF_FILE}  # Now depends on the ref file as well.
        COMMENT "Generating buildinfo.cpp with updated Git info"
        VERBATIM
)

add_custom_target(update_git_version ALL DEPENDS ${BUILDINFO_CPP})

# Toggle AVX2 for Glaze based on build type
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(glaze_ENABLE_AVX2 OFF CACHE BOOL "Disable AVX2 for Glaze in Debug mode" FORCE)
else()
    set(glaze_ENABLE_AVX2 ON CACHE BOOL "Enable AVX2 for Glaze in non-Debug modes" FORCE)
endif()

# External dependencies
add_subdirectory(external/portaudio)
add_subdirectory(external/json)
add_subdirectory(external/linenoise-ng)
add_subdirectory(external/unordered_dense)
add_subdirectory(external/glaze)
add_subdirectory(external/SDL2)
add_subdirectory(external/nanovg)
add_subdirectory(external/rtmidi)

# Enable static SDL2 build
#set(SDL_SHARED OFF CACHE BOOL "Disable SDL shared library" FORCE)
#set(SDL_STATIC ON CACHE BOOL "Enable SDL static library" FORCE)
#set(SDL_AUDIO OFF CACHE BOOL "Disable SDL audio support" FORCE)
#set(SDL_JOYSTICK OFF CACHE BOOL "Disable SDL joystick support" FORCE)
#set(SDL_HAPTIC OFF CACHE BOOL "Disable SDL haptic support" FORCE)
#set(SDL_RENDER OFF CACHE BOOL "Disable SDL renderer support" FORCE)
#set(SDL_RENDER_GPU ON CACHE BOOL "Enable GPU support" FORCE)

# Add moodycamel concurrentqueue
add_library(moodycamel_concurrentqueue INTERFACE)
target_include_directories(moodycamel_concurrentqueue INTERFACE "${CMAKE_SOURCE_DIR}/external/concurrentqueue")

add_library(pffft STATIC external/pffft/pffft.c)
target_include_directories(pffft PUBLIC external/pffft)

add_library(glad STATIC Glad/glad.cpp)
target_include_directories(glad PUBLIC "${CMAKE_SOURCE_DIR}/Glad")

# Collect common source files
file(GLOB_RECURSE GRAPH_SOURCES "${CMAKE_SOURCE_DIR}/Graph/*.cpp" "${CMAKE_SOURCE_DIR}/Utility/*.cpp")

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
add_executable(PatchformStandalone PlugPatchStandalone.cpp ${GRAPH_SOURCES} ${UI_SOURCES} ${BUILDINFO_CPP})
target_compile_definitions(PatchformStandalone PRIVATE PATCHFORM_WITH_GUI NANOVG_GL3_IMPLEMENTATION=1)
target_include_directories(PatchformStandalone PRIVATE
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/external/concurrentqueue"
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
)
target_link_libraries(PatchformStandalone PRIVATE
        SDL3::SDL3-static
        portaudio
        moodycamel_concurrentqueue
        unordered_dense
        glaze::glaze
        glad
        nanovg
        pffft
        rtmidi
)
target_compile_definitions(PatchformStandalone PRIVATE)

# Apply optimizations for MSVC
if (MSVC)
    add_link_options($<$<CONFIG:Debug>:/DEBUG>)

    target_compile_options(PatchformStandalone PRIVATE
            /std:c++latest
            /Zc:preprocessor
            $<$<CONFIG:Debug>:/Od /Zi /RTC1>
            $<$<CONFIG:Release>:/Ox /arch:AVX2 /GL /GS- /Ob3 /fp:fast>
            $<$<CONFIG:RelWithDebInfo>:/Ox /Z7 /GL>

    )
endif ()

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
    # Add Windows-specific resources
    set(APP_ICON_RESOURCE "installer/windows/resources.rc")
    target_sources(PatchformStandalone PRIVATE ${APP_ICON_RESOURCE})
endif ()

# files to copy
set(SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/Patches/graph.json"
        "${CMAKE_SOURCE_DIR}/Patches/graph1.json"
        "${CMAKE_SOURCE_DIR}/Patches/graph2.json"
        "${CMAKE_SOURCE_DIR}/Patches/graph3.json"
        "${CMAKE_SOURCE_DIR}/Patches/graph4.json5"
        "${CMAKE_SOURCE_DIR}/Patches/count.json5"
        "${CMAKE_SOURCE_DIR}/Patches/Inter-VariableFont_opsz,wght.ttf"
        "${CMAKE_SOURCE_DIR}/Assets/Icons/IconFontPlugPatch.ttf"
        "${CMAKE_SOURCE_DIR}/Assets/Icons/IconFontPlugPatch_google.ttf"
)

# Define a custom target to copy the Patches directory
add_custom_target(copy_patches
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Patches"
        "$<TARGET_FILE_DIR:PlugPatchCLI>/Patches"
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
add_dependencies(PatchformStandalone copy_patches copy_fonts update_git_version)

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
            COMMAND makensis.exe /DOUTPUT_DIR=$<TARGET_FILE_DIR:PatchformStandalone> /DAPPLICATION_BINARY=${APPLICATION_BINARY_STANDALONE} ${NSIS_SCRIPT}
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            COMMENT "Packaging installer with NSIS"
            VERBATIM
    )
    add_dependencies(PackageInstallerStandalone PatchformStandalone)
else()
    message(WARNING "The PackageInstallers target is only available in Release mode.")
endif()
