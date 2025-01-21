cmake_minimum_required(VERSION 3.30)
project(PlugPatch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

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

add_library(moodycamel_concurrentqueue INTERFACE)
target_include_directories(moodycamel_concurrentqueue INTERFACE "${CMAKE_SOURCE_DIR}/external/concurrentqueue")

# Collect common source files
file(GLOB ROOT_SOURCES "*.cpp")
file(GLOB_RECURSE GRAPH_SOURCES "${CMAKE_SOURCE_DIR}/Graph/*.cpp")
set(COMMON_SOURCES ${ROOT_SOURCES} ${GRAPH_SOURCES})

# Fix for linenoise not being updated to C++20
target_compile_options(linenoise PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:char8_t->
        $<$<CXX_COMPILER_ID:GNU>:-fno-char8_t>
        $<$<CXX_COMPILER_ID:Clang>:-fno-char8_t>
)

# Include directories
set(INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/external/concurrentqueue"
        "${CMAKE_SOURCE_DIR}/external/json/single_include/nlohmann"
        "${CMAKE_SOURCE_DIR}/external/linenoise-ng/include"
        "${CMAKE_SOURCE_DIR}/external/portaudio/include"
        "${CMAKE_SOURCE_DIR}/external/unordered_dense/include/ankerl"
        "${CMAKE_SOURCE_DIR}/external/glaze/include/"
)

# Function to create a target
function(create_target target_name source_file additional_sources)
    add_executable(${target_name} ${source_file} ${COMMON_SOURCES} ${additional_sources})
    target_include_directories(${target_name} PRIVATE ${INCLUDE_DIRS})
    target_link_libraries(${target_name} PRIVATE portaudio linenoise moodycamel_concurrentqueue unordered_dense glaze::glaze)

    # Apply optimizations for MSVC
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
                /std:c++latest
                /Zc:preprocessor
                $<$<CONFIG:Debug>:/Od /Z7>
                $<$<CONFIG:Release>:/Ox /arch:AVX2 /GL /GS- /Ob3 /fp:fast>
                $<$<CONFIG:RelWithDebInfo>:/Ox /Z7 /GL>
        )
    endif()

    if(WIN32)
        # Add Windows-specific resources
        set(APP_ICON_RESOURCE "installer/windows/resources.rc")
        target_sources(${target_name} PRIVATE ${APP_ICON_RESOURCE})
    endif()
endfunction()

# Create targets
create_target(PlugPatchCLI "PlugPatchCLI.cpp" "")
create_target(PlugPatchStandalone "PlugPatchStandalone.cpp" "")

# Copy JSON assets
set(SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/graph.json"
        "${CMAKE_SOURCE_DIR}/graph1.json"
        "${CMAKE_SOURCE_DIR}/graph2.json"
        "${CMAKE_SOURCE_DIR}/graph3.json"
        "${CMAKE_SOURCE_DIR}/graph4.json5"
        "${CMAKE_SOURCE_DIR}/count.json5"
)

foreach(source_file IN LISTS SOURCE_FILES)
    get_filename_component(file_name "${source_file}" NAME)
    set(dest_file "${CMAKE_BINARY_DIR}/$<CONFIG>/${file_name}")

    foreach(target IN LISTS PlugPatchCLI PlugPatchStandalone)
        add_custom_command(
                TARGET ${target}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${source_file}"
                "${dest_file}"
                COMMENT "Copying ${file_name} to ${dest_file} for target ${target}"
        )
    endforeach()
endforeach()

# Installer configuration for standalone
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(INSTALLER_DIR ${CMAKE_SOURCE_DIR}/installer)
    set(NSIS_SCRIPT ${INSTALLER_DIR}/windows/installer.nsi)

    add_custom_target(
            PackageInstaller ALL
            COMMAND ${CMAKE_COMMAND} -E echo "Creating NSIS installer..."
            COMMAND makensis.exe /DOUTPUT_DIR=${CMAKE_BINARY_DIR}/Release ${NSIS_SCRIPT}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Packaging installer with NSIS"
            VERBATIM
    )
    add_dependencies(PackageInstaller PlugPatchStandalone)
else()
    message(WARNING "The PackageInstaller target is only available in Release mode.")
endif()
