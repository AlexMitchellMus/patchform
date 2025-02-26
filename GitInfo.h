#pragma once

// Access the git verion / hash from the generated GitInfo.cpp file
// We do this so that it is updated when the git version/hash changes

// See GenerateGitInfo.cmake to see how the cpp is generated

extern const char* const patchform_git_version;
extern const char* const patchform_git_hash;
