cmake_minimum_required(VERSION 3.30)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
endif()

project(Patchform LANGUAGES C CXX)

option(ENABLE_ASAN "Enable Address Sanitizer" ON)
option(RUN_TESTS "Build and run tests" OFF)

set(CMAKE_POLICY_VERSION 3.5)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if (MSVC)
    if (ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL" CACHE STRING "" FORCE)
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
    endif()
endif()

set(BUILDINFO_CPP ${CMAKE_BINARY_DIR}/GitInfo.cpp)
file(READ "${CMAKE_SOURCE_DIR}/.git/HEAD" HEAD_CONTENTS)
string(REGEX MATCH "ref: (.*)" _ ${HEAD_CONTENTS})
if(CMAKE_MATCH_1)
    set(GIT_REF_FILE "${CMAKE_SOURCE_DIR}/.git/${CMAKE_MATCH_1}")
    message(STATUS "Git ref file: ${GIT_REF_FILE}")
else()
    set(GIT_REF_FILE "${CMAKE_SOURCE_DIR}/.git/HEAD")
endif()

set(GIT_VERSION_FILE "${CMAKE_BINARY_DIR}/GitVersion.txt")

add_custom_command(
        OUTPUT ${BUILDINFO_CPP}
        COMMAND ${CMAKE_COMMAND} -DOUTPUT_FILE=${BUILDINFO_CPP} -DGIT_VERSION_FILE=${GIT_VERSION_FILE} -P ${CMAKE_SOURCE_DIR}/GenerateGitInfo.cmake
        DEPENDS ${CMAKE_SOURCE_DIR}/.git/HEAD ${GIT_REF_FILE}
        COMMENT "Generating buildinfo.cpp with updated Git info"
        VERBATIM
)
add_custom_target(update_git_version ALL DEPENDS ${BUILDINFO_CPP})

if (EXISTS "${CMAKE_BINARY_DIR}/GitVersion.txt")
    file(READ "${CMAKE_BINARY_DIR}/GitVersion.txt" PATCHFORM_VERSION_STRING)
    string(STRIP "${PATCHFORM_VERSION_STRING}" PATCHFORM_VERSION_STRING)
else()
    set(PATCHFORM_VERSION_STRING "dev")
endif()
message(STATUS "Detected Patchform version: ${PATCHFORM_VERSION_STRING}")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+).*" "\\1,\\2,\\3,0" PATCHFORM_VERSION_COMMA "${PATCHFORM_VERSION_STRING}")

if(WIN32)
    add_custom_command(
            OUTPUT "${CMAKE_BINARY_DIR}/patchformicon.ico"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/installer/windows/patchformicon.ico" "${CMAKE_BINARY_DIR}/patchformicon.ico"
            DEPENDS "${CMAKE_SOURCE_DIR}/installer/windows/patchformicon.ico"
            COMMENT "Copying patchformicon.ico"
    )
    add_custom_target(copy_icon ALL DEPENDS "${CMAKE_BINARY_DIR}/patchformicon.ico")
    set(PATCHFORM_ICON_PATH "${CMAKE_BINARY_DIR}/patchformicon.ico")
    configure_file(${CMAKE_SOURCE_DIR}/installer/windows/resources.rc.in ${CMAKE_BINARY_DIR}/version.rc @ONLY)
endif()

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

add_subdirectory(external/clap)

add_subdirectory(Nodes/nodes/Metronome)

# Add moodycamel concurrentqueue
add_library(moodycamel_concurrentqueue INTERFACE)
target_include_directories(moodycamel_concurrentqueue INTERFACE "${CMAKE_SOURCE_DIR}/external/concurrentqueue")

# Add moodycamel readerwriterqueue
add_library(moodycamel_readerwriterqueue INTERFACE)
target_include_directories(moodycamel_readerwriterqueue INTERFACE "${CMAKE_SOURCE_DIR}/external/readerwriterqueue")

add_library(pffft OBJECT external/pffft/pffft.c)
target_include_directories(pffft PUBLIC external/pffft)

target_compile_options(pffft PRIVATE
        #$<$<CXX_COMPILER_ID:MSVC>:
        #$<$<CONFIG:Release>:/Ox /arch:AVX2 /fp:fast /Ob3 /GL /DNDEBUG>
        #$<$<CONFIG:Debug>:/Od /Zi>
)

add_library(glad STATIC Glad/glad.cpp)
target_include_directories(glad PUBLIC "${CMAKE_SOURCE_DIR}/Glad")

add_library(NanoVGWrapper OBJECT
        ${CMAKE_SOURCE_DIR}/Glad/NanoVGWrapper.cpp
        ${CMAKE_SOURCE_DIR}/external/nanovg/src/nanovg.cpp
)

target_include_directories(NanoVGWrapper PUBLIC
        ${CMAKE_SOURCE_DIR}/Glad
        ${CMAKE_SOURCE_DIR}/external/nanovg/src
)

target_link_libraries(NanoVGWrapper PRIVATE glad)
target_compile_definitions(NanoVGWrapper PRIVATE NANOVG_GL3_IMPLEMENTATION)

file(GLOB_RECURSE GRAPH_SOURCES "${CMAKE_SOURCE_DIR}/Graph/*.cpp" "${CMAKE_SOURCE_DIR}/Utility/*.cpp")
file(GLOB NODE_SHARED_SOURCES "${CMAKE_SOURCE_DIR}/Nodes/*.cpp")
file(GLOB_RECURSE UI_SOURCES "${CMAKE_SOURCE_DIR}/UI/*.cpp" "${CMAKE_SOURCE_DIR}/UI_ToolKit/*.cpp")

# Select platform-specific implementation
if(APPLE)
    set(PLATFORM_HELPERS ${CMAKE_SOURCE_DIR}/PlatformHelpers/PlatformHelpers.mm)
else()
    set(PLATFORM_HELPERS ${CMAKE_SOURCE_DIR}/PlatformHelpers/PlatformHelpers.cpp)
endif()

add_library(PatchformSDK SHARED
        ${GRAPH_SOURCES}
        ${NODE_SHARED_SOURCES}
        ${UI_SOURCES}
        ${BUILDINFO_CPP}
        ${PLATFORM_HELPERS}
        $<TARGET_OBJECTS:NanoVGWrapper>
        UI_ToolKit/PluginWindowPeer.cpp
        UI_ToolKit/PluginWindowPeer_mac.mm
)

target_include_directories(PatchformSDK PUBLIC
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/Glad"
        "${CMAKE_SOURCE_DIR}/external"
        "${CMAKE_SOURCE_DIR}/external/concurrentqueue"
        "${CMAKE_SOURCE_DIR}/external/readerwriterqueue"
        "${CMAKE_SOURCE_DIR}/external/json/single_include/nlohmann"
        "${CMAKE_SOURCE_DIR}/external/unordered_dense/include/ankerl"
        "${CMAKE_SOURCE_DIR}/external/glaze/include"
        "${CMAKE_SOURCE_DIR}/external/portaudio/include"
        "${CMAKE_SOURCE_DIR}/external/nanovg/src"
        "${CMAKE_SOURCE_DIR}/external/rtmidi"
        "${CMAKE_SOURCE_DIR}/external/pffft"
        "${CMAKE_SOURCE_DIR}/external/SDL2/include"
        "${CMAKE_SOURCE_DIR}/external/libsamplerate/include"
        "${CMAKE_SOURCE_DIR}/external/simde"
        "${CMAKE_SOURCE_DIR}/external/SignalsmithDSP/include"
)

target_link_libraries(PatchformSDK PRIVATE
        SDL3::SDL3-static
        portaudio
        moodycamel_concurrentqueue
        moodycamel_readerwriterqueue
        unordered_dense
        glaze::glaze
        rtmidi
        glad
        samplerate
        pffft
)

target_compile_definitions(PatchformSDK PRIVATE
        LIBSAMPLERATE_STATIC
        NANOVG_GL3_IMPLEMENTATION=1

        PATCHFORM_WITH_GUI
        PATCHFORM_SDK_BUILD
        #PATCHFORM_DEBUG_TILE_REPAINT
)

add_dependencies(PatchformSDK update_git_version)

#=== PLUGIN ===

add_library(PatchformCLAP MODULE
        PatchformCLAP.cpp
        PatchformClapPlugin.cpp
        PatchformClapPluginGUI.mm
        PatchformGLView.mm
        PatchformBuildMode.cpp
)

if(APPLE)
    find_library(APPKIT_FRAMEWORK AppKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)

    target_link_libraries(PatchformCLAP PRIVATE
            ${APPKIT_FRAMEWORK}
            ${COREVIDEO_FRAMEWORK}
    )
endif()

set_target_properties(PatchformCLAP PROPERTIES
        BUNDLE True
        MACOSX_BUNDLE TRUE
        BUNDLE_EXTENSION "clap"
        SUFFIX ""  # no .clap
        MACOSX_BUNDLE_GUI_IDENTIFIER com.my_company.my_plug
        MACOSX_BUNDLE_BUNDLE_NAME PatchformCLAP
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/installer/macOS/plugin/Info.plist.in"
        POSITION_INDEPENDENT_CODE TRUE
)

if(${CMAKE_CXX_COMPILER_ID} MATCHES "AppleClang|Clang|GNU")
    add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden
            -Werror=return-type -Werror=unused-result -Wno-unused-parameter
            -Wuninitialized -Wvla -Wmultichar -Werror=non-virtual-dtor -Woverloaded-virtual)
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "AppleClang|Clang")
    add_compile_options(-Wmost -Wsuper-class-method-mismatch)
endif()

target_include_directories(PatchformCLAP PRIVATE
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/Glad"
        "${CMAKE_SOURCE_DIR}/external"
        "${CMAKE_SOURCE_DIR}/external/concurrentqueue"
        "${CMAKE_SOURCE_DIR}/external/readerwriterqueue"
        "${CMAKE_SOURCE_DIR}/external/json/single_include/nlohmann"
        "${CMAKE_SOURCE_DIR}/external/unordered_dense/include/ankerl"
        "${CMAKE_SOURCE_DIR}/external/glaze/include"
        "${CMAKE_SOURCE_DIR}/external/portaudio/include"
        "${CMAKE_SOURCE_DIR}/external/nanovg/src"
        "${CMAKE_SOURCE_DIR}/external/SDL2/include"
        "${CMAKE_SOURCE_DIR}/external/libsamplerate/include"
        "${CMAKE_SOURCE_DIR}/external/pffft"
        "${CMAKE_SOURCE_DIR}/external/simde"
        "${CMAKE_SOURCE_DIR}/external/SignalsmithDSP/include"
        "${CMAKE_SOURCE_DIR}/external/clap/include"
)

target_compile_definitions(PatchformCLAP PRIVATE
        PATCHFORM_PLUGIN
        SDL_MAIN_USE_CALLBACKS=1
        PATCHFORM_WITH_GUI
        PATCHFORM_STANDALONE
)

target_link_libraries(PatchformCLAP PRIVATE PatchformSDK)

add_dependencies(PatchformCLAP PatchformSDK)

configure_file(
        ${CMAKE_SOURCE_DIR}/installer/macOS/plugin/Info.plist.in
        ${CMAKE_BINARY_DIR}/GeneratedInfo.plist
        @ONLY
)

add_custom_command(TARGET PatchformCLAP POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Ensuring Info.plist is present in .clap bundle"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:PatchformCLAP>/Contents"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/GeneratedInfo.plist"
        "$<TARGET_BUNDLE_DIR:PatchformCLAP>/Contents/Info.plist"
)

target_link_options(PatchformCLAP PRIVATE
        -exported_symbols_list ${CMAKE_SOURCE_DIR}/installer/macOS/plugin/exported_symbols.txt
)

if (APPLE)
    add_custom_command(TARGET PatchformCLAP POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:PatchformCLAP>/Contents/Resources/Assets/Fonts"
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:PatchformCLAP>/Contents/Resources/Assets/Icons"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets/Fonts"
            "$<TARGET_BUNDLE_DIR:PatchformCLAP>/Contents/Resources/Assets/Fonts"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets/Icons"
            "$<TARGET_BUNDLE_DIR:PatchformCLAP>/Contents/Resources/Assets/Icons"
            COMMENT "Copying Fonts and Icons to .clap bundle Resources/Assets"
    )
endif ()

#===  STANDALONE ===

add_executable(PatchformStandalone
        PatchformStandalone.cpp
        PatchformBuildMode.cpp
        UI_ToolKit/SDLWindowPeer.cpp)

if(APPLE)
    if(CMAKE_BUILD_TYPE MATCHES Release)
        target_compile_options(PatchformSDK PRIVATE -O3 -march=native -ffast-math)
        target_compile_options(PatchformStandalone PRIVATE -O3 -march=native -ffast-math)
        target_compile_options(PatchformCLAP PRIVATE -O3 -march=native -ffast-math)
    endif()

    set(MACOSX_BUNDLE TRUE)
    set(MACOSX_BUNDLE_ICON_FILE patchformicon.icns)

    set_source_files_properties(${CMAKE_SOURCE_DIR}/installer/macOS/patchformicon.icns
            PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    set_target_properties(PatchformStandalone PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/installer/macOS/Info.plist"
    )

    target_sources(PatchformStandalone PRIVATE
            ${CMAKE_SOURCE_DIR}/installer/macOS/patchformicon.icns
    )
endif()

target_include_directories(PatchformStandalone PRIVATE
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/Glad"
        "${CMAKE_SOURCE_DIR}/external"
        "${CMAKE_SOURCE_DIR}/external/concurrentqueue"
        "${CMAKE_SOURCE_DIR}/external/readerwriterqueue"
        "${CMAKE_SOURCE_DIR}/external/json/single_include/nlohmann"
        "${CMAKE_SOURCE_DIR}/external/unordered_dense/include/ankerl"
        "${CMAKE_SOURCE_DIR}/external/glaze/include"
        "${CMAKE_SOURCE_DIR}/external/portaudio/include"
        "${CMAKE_SOURCE_DIR}/external/nanovg/src"
        "${CMAKE_SOURCE_DIR}/external/SDL2/include"
        "${CMAKE_SOURCE_DIR}/external/libsamplerate/include"
        "${CMAKE_SOURCE_DIR}/external/simde"
        "${CMAKE_SOURCE_DIR}/external/SignalsmithDSP/include"
)

target_link_libraries(PatchformStandalone PRIVATE PatchformSDK)

target_compile_definitions(PatchformStandalone PRIVATE
        SDL_MAIN_USE_CALLBACKS=1
        PATCHFORM_WITH_GUI
        PATCHFORM_STANDALONE
)

# For macOS package-related targets, skip Windows-specific packaging
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(INSTALLER_DIR ${CMAKE_SOURCE_DIR}/installer)

    set(NSIS_SCRIPT ${INSTALLER_DIR}/windows/installer.nsi)
    set(APPLICATION_BINARY_CLI "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/PlugPatchCLI.exe")
    add_custom_target(PackageInstaller
            COMMAND ${CMAKE_COMMAND} -E echo "Creating NSIS installer..."
            COMMAND makensis.exe /DOUTPUT_DIR=$<TARGET_FILE_DIR:PlugPatchCLI> /DAPPLICATION_BINARY=${APPLICATION_BINARY_CLI} ${NSIS_SCRIPT}
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            COMMENT "Packaging installer with NSIS"
            VERBATIM)
    add_dependencies(PackageInstaller PlugPatchCLI)

    set(NSIS_SCRIPT ${INSTALLER_DIR}/windows/installerStandalone.nsi)
    set(APPLICATION_BINARY_STANDALONE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/PatchformStandalone.exe")
    add_custom_target(PackageInstallerStandalone
            COMMAND ${CMAKE_COMMAND} -E echo "Creating NSIS installer..."
            COMMAND makensis.exe /DOUTPUT_DIR=$<TARGET_FILE_DIR:PatchformStandalone> /DAPPLICATION_BINARY=${APPLICATION_BINARY_STANDALONE} /DAPP_VERSION=${PATCHFORM_VERSION_STRING} ${NSIS_SCRIPT}
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            COMMENT "Packaging installer with NSIS"
            VERBATIM)
    add_dependencies(PackageInstallerStandalone PatchformStandalone)
else()
    message(STATUS "Skipping Windows installer packaging on non-Windows platform or non-Release build.")
endif ()

if (APPLE)
    add_custom_command(TARGET PatchformStandalone POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:PatchformStandalone>/Contents/Resources/Assets/Fonts"
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:PatchformStandalone>/Contents/Resources/Assets/Icons"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets/Fonts"
            "$<TARGET_BUNDLE_DIR:PatchformStandalone>/Contents/Resources/Assets/Fonts"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets/Icons"
            "$<TARGET_BUNDLE_DIR:PatchformStandalone>/Contents/Resources/Assets/Icons"
            COMMENT "Copying Fonts and Icons to .app Resources/Assets"
    )
else ()
    add_custom_command(TARGET PatchformStandalone POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:PatchformStandalone>/Assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets/Fonts"
            "$<TARGET_FILE_DIR:PatchformStandalone>/Assets/Fonts"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets/Icons"
            "$<TARGET_FILE_DIR:PatchformStandalone>/Assets/Icons"
            COMMENT "Copying Fonts and Icons to bin/Assets"
    )
endif ()
