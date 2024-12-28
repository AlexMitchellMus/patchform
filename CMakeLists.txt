cmake_minimum_required(VERSION 3.30)
project(audiograph)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(external/portaudio)

add_executable(audiograph main.cpp)

target_link_libraries(audiograph PRIVATE portaudio)