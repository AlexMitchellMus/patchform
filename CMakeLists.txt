cmake_minimum_required(VERSION 3.30)
project(audiograph)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(external/portaudio)
add_subdirectory(external/json)

file(GLOB SOURCES "*.cpp")
add_executable(audiograph ${SOURCES})

target_link_libraries(audiograph PRIVATE portaudio)

# Apply optimizations for MSVC
if(MSVC)
    # Use /O2 for high optimization in MSVC
    target_compile_options(audiograph PRIVATE /O3)
    # Optionally enable other MSVC-specific optimizations
    target_compile_options(audiograph PRIVATE /arch:AVX2)  # For AVX2 if supported by your CPU
    # You can also add other flags like /GL for whole program optimization if necessary
    target_compile_options(audiograph PRIVATE /GL)
endif()

# List all the JSON files you want to copy
set(SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/graph.json"
        "${CMAKE_SOURCE_DIR}/graph2.json"
        "${CMAKE_SOURCE_DIR}/graph3.json"
        "${CMAKE_SOURCE_DIR}/graph4.json5"
)

# Loop over them and create a post-build copy command for each
foreach(source_file IN LISTS SOURCE_FILES)
    # Extract just the filename so we can construct the destination path
    get_filename_component(file_name "${source_file}" NAME)
    set(dest_file "${CMAKE_BINARY_DIR}/$<CONFIG>/${file_name}")

    add_custom_command(
            TARGET audiograph
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${source_file}"
            "${dest_file}"
            COMMENT "Copying ${file_name} to ${dest_file}"
    )
endforeach()