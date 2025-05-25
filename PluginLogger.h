//
// Created by Alexander Mitchell on 22/5/2025.
//


// Just print the debug to file, wasting time trying to get hosts to print their stdout is mind-numbing
// Open in terminal - make sure the plugin has init first to generate the file!
//  tail -f /tmp/patchform_debug.log
//

#pragma once

//#define PATCHFORM_ENABLE_LOG
#ifdef PATCHFORM_ENABLE_LOG
    #include <fstream>
    #define LOG_TO_FILE(msg)                                      \
    do {                                                          \
    std::ofstream log("/tmp/patchform_debug.log", std::ios::app); \
    log << msg << std::endl;                                      \
    } while (0)
#else
    #define LOG_TO_FILE(msg)
#endif