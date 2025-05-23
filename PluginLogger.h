//
// Created by Alexander Mitchell on 22/5/2025.
//

#pragma once

#include <fstream>

// Just print the debug to file, wasting time trying to get hosts to print their stdout is mind-numbing
// Open in terminal - make sure the plugin has init first to generate the file!
//  tail -f /tmp/patchform_debug.log
//
static void logToFile(const std::string& msg) {
    std::ofstream log("/tmp/patchform_debug.log", std::ios::app);
    log << msg << std::endl;
}