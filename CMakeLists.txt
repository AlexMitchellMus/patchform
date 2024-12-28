cmake_minimum_required(VERSION 3.30)
project(audiograph)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(external/portaudio)

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