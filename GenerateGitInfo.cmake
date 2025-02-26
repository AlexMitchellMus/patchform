# GenerateGitInfo.cmake
#
# This script executes git to obtain the current commit hash and writes
# a C++ source snippet that defines a variable with the Git hash.
#
# The output file is passed in as a variable OUTPUT_FILE.

# Run git and capture the commit hash.
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
        COMMAND git describe --tags --abbrev=0
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Write the generated file.
file(WRITE "${OUTPUT_FILE}" "#include \"GitInfo.h\"\n")
file(APPEND "${OUTPUT_FILE}" "const char* const patchform_git_version = \"${GIT_VERSION}\";\n")
file(APPEND "${OUTPUT_FILE}" "const char* const patchform_git_hash = \"${GIT_HASH}\";\n")

