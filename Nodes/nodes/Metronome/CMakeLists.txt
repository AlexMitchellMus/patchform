cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_library(Metronome SHARED Metronome.cpp)

if (MSVC)
    target_compile_options(Metronome PRIVATE
            /std:c++latest
            /Zc:preprocessor
            $<$<CONFIG:Debug>:/Od /Zi /RTC1 /bigobj /Zc:__cplusplus>
            $<$<CONFIG:Release>:/Ox /arch:AVX2 /GL /GS- /Ob3 /fp:fast /bigobj /DNDEBUG>
            $<$<CONFIG:RelWithDebInfo>:/Ox /Z7 /GL /bigobj>
            )
endif()

target_include_directories(Metronome PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/UI
        ${PROJECT_SOURCE_DIR}/UI_ToolKit

        ${PROJECT_SOURCE_DIR}/external/nanovg/src
        ${PROJECT_SOURCE_DIR}/external/readerwriterqueue
        ${PROJECT_SOURCE_DIR}/external/concurrentqueue
        ${PROJECT_SOURCE_DIR}/external/json/single_include/nlohmann
        ${PROJECT_SOURCE_DIR}/external/unordered_dense/include/
        ${CMAKE_SOURCE_DIR}/external/unordered_dense/include/ankerl
        ${PROJECT_SOURCE_DIR}/Glad/
        ${PROJECT_SOURCE_DIR}/external/glaze/include
        ${PROJECT_SOURCE_DIR}/external/SDL2/include
)

target_link_libraries(Metronome PRIVATE
        PatchformSDK
)

target_compile_definitions(Metronome PRIVATE PATCHFORM_WITH_GUI PATCHFORM_PLUGIN_BUILD NANOVG_GL3_IMPLEMENTATION=1)

set_target_properties(Metronome PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG         ${CMAKE_BINARY_DIR}/objects
        RUNTIME_OUTPUT_DIRECTORY_RELEASE       ${CMAKE_BINARY_DIR}/objects
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/objects
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL    ${CMAKE_BINARY_DIR}/objects
)