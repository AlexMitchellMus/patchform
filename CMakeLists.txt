cmake_minimum_required(VERSION 3.30)
project(PlugPatch)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(external/portaudio)
add_subdirectory(external/json)
add_subdirectory(external/linenoise-ng)

add_library(moodycamel_concurrentqueue INTERFACE)
target_include_directories(moodycamel_concurrentqueue INTERFACE "${CMAKE_SOURCE_DIR}/external/concurrentqueue")

# Collect .cpp files in the current directory
file(GLOB ROOT_SOURCES "*.cpp")

# Recursively collect .cpp files from the Graph subdirectory
file(GLOB_RECURSE GRAPH_SOURCES "${CMAKE_SOURCE_DIR}/Graph/*.cpp")

# Combine all sources
set(SOURCES ${ROOT_SOURCES} ${GRAPH_SOURCES})

# Add executable target
add_executable(PlugPatch ${SOURCES})

# fix for linenoise not being updated to C++20
target_compile_options(linenoise PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:char8_t->
        $<$<CXX_COMPILER_ID:GNU>:-fno-char8_t>
        $<$<CXX_COMPILER_ID:Clang>:-fno-char8_t>
)

target_include_directories(PlugPatch PRIVATE
        "${CMAKE_SOURCE_DIR}/external/concurrentqueue"
        "${CMAKE_SOURCE_DIR}/external/json/single_include/nlohmann"
        "${CMAKE_SOURCE_DIR}/external/linenoise-ng/include"
        "${CMAKE_SOURCE_DIR}/external/portaudio/include"
)

target_link_libraries(PlugPatch PRIVATE portaudio linenoise moodycamel_concurrentqueue)

# Ensure linenoise is built before PlugPatch
add_dependencies(PlugPatch linenoise)

# Apply optimizations for MSVC
if(MSVC)
    target_compile_options(PlugPatch PRIVATE
            $<$<CONFIG:Debug>:/Od /Z7>
            $<$<CONFIG:Release>:/Ox /arch:AVX2 /GL>
            $<$<CONFIG:RelWithDebInfo>:/Ox /Z7 /GL>
    )
endif()

# patch assets to copy
set(SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/graph.json"
        "${CMAKE_SOURCE_DIR}/graph1.json"
        "${CMAKE_SOURCE_DIR}/graph2.json"
        "${CMAKE_SOURCE_DIR}/graph3.json"
        "${CMAKE_SOURCE_DIR}/graph4.json5"
        "${CMAKE_SOURCE_DIR}/count.json5"
)

# Loop over them and create a post-build copy command for each
foreach(source_file IN LISTS SOURCE_FILES)
    # Extract just the filename so we can construct the destination path
    get_filename_component(file_name "${source_file}" NAME)
    set(dest_file "${CMAKE_BINARY_DIR}/$<CONFIG>/${file_name}")

    add_custom_command(
            TARGET PlugPatch
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${source_file}"
            "${dest_file}"
            COMMENT "Copying ${file_name} to ${dest_file}"
    )
endforeach()