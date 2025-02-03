cmake_minimum_required(VERSION 3.30)
project(PlugPatch LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

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

# Collect common source files
file(GLOB_RECURSE GRAPH_SOURCES "${CMAKE_SOURCE_DIR}/Graph/*.cpp")

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
add_executable(PlugPatchStandalone PlugPatchStandalone.cpp ${GRAPH_SOURCES} ${UI_SOURCES})
target_compile_definitions(PlugPatchStandalone PRIVATE PATCHFORM_WITH_GUI)
target_include_directories(PlugPatchStandalone PRIVATE
        "${CMAKE_SOURCE_DIR}/external/concurrentqueue"
        "${CMAKE_SOURCE_DIR}/external/json/single_include/nlohmann"
        "${CMAKE_SOURCE_DIR}/external/unordered_dense/include/ankerl"
        "${CMAKE_SOURCE_DIR}/external/glaze/include/"
        "${CMAKE_SOURCE_DIR}/external/portaudio/include"
        "${CMAKE_SOURCE_DIR}/external/nanovg/src/"
        "${CMAKE_SOURCE_DIR}/Glad/"
        "${CMAKE_SOURCE_DIR}/external/SDL2/include"
)
target_link_libraries(PlugPatchStandalone PRIVATE
        SDL3::SDL3-static
        portaudio
        moodycamel_concurrentqueue
        unordered_dense
        glaze::glaze
        nanovg
)
target_compile_definitions(PlugPatchStandalone PRIVATE)

set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DEBUG")


# Apply optimizations for MSVC
if (MSVC)
    target_compile_options(PlugPatchStandalone PRIVATE
            /std:c++latest
            /Zc:preprocessor
            $<$<CONFIG:Debug>:/Od /Zi /RTC1>
            $<$<CONFIG:Release>:/Ox /arch:AVX2 /GL /GS- /Ob3 /fp:fast>
            $<$<CONFIG:RelWithDebInfo>:/Ox /Z7 /GL>
    )
endif ()

if (WIN32)
    # Add Windows-specific resources
    set(APP_ICON_RESOURCE "installer/windows/resources.rc")
    target_sources(PlugPatchStandalone PRIVATE ${APP_ICON_RESOURCE})
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
        "$<TARGET_FILE_DIR:PlugPatchStandalone>/Assets"
        COMMENT "Copying Patches directory to the output directory"
)

# Add dependencies to ensure the directory is copied whenever these targets are built
add_dependencies(PlugPatchCLI copy_patches)
add_dependencies(PlugPatchStandalone copy_patches copy_fonts)

# Installer configuration for CLI
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(INSTALLER_DIR ${CMAKE_SOURCE_DIR}/installer)
    set(NSIS_SCRIPT ${INSTALLER_DIR}/windows/installer.nsi)

    # Define the location of the built application
    set(APPLICATION_BINARY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/PlugPatchCLI.exe")

    add_custom_target(PackageInstaller
            COMMAND ${CMAKE_COMMAND} -E echo "Creating NSIS installer..."
            COMMAND makensis.exe /DOUTPUT_DIR=$<TARGET_FILE_DIR:PlugPatchCLI> /DAPPLICATION_BINARY=${APPLICATION_BINARY} ${NSIS_SCRIPT}
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            COMMENT "Packaging installer with NSIS"
            VERBATIM
    )
    add_dependencies(PackageInstaller PlugPatchCLI)
else()
    message(WARNING "The PackageInstaller target is only available in Release mode.")
endif()
