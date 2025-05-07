add_library(Metronome SHARED Metronome.cpp)
target_include_directories(Metronome PRIVATE ${PROJECT_SOURCE_DIR}/Nodes)
target_compile_definitions(Metronome PRIVATE PATCHFORM_PLUGIN_BUILD)

set_target_properties(Metronome PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG     ${PROJECT_SOURCE_DIR}/objects
        RUNTIME_OUTPUT_DIRECTORY_RELEASE   ${PROJECT_SOURCE_DIR}/objects
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_SOURCE_DIR}/objects
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${PROJECT_SOURCE_DIR}/objects
)


