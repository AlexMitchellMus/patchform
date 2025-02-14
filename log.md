Project Goals

* Sample accurate events
* Similar node logic to Puredata
* Separation of audio / graph maintenance
* No clicking ever, for live patching etc (graph discontinuity is ok, as some graph changes will be impossible to mitigate - clicking due to priority inversion is not ok)
* Zero allocation on audio thread
* Simple / logical, and expressive file format (using json)
* Dynamic loading of compiled externals
* Sub-patches
* Multi-patch definitions (define multiple patches in same file, reuse the patch locally)
* Modern c++, make library as simple as possible.
* Set values for nodes from external controls (UI etc)
* Expressive and feature rich ABI for external / plugin c++ creation

[ PROJECT ROADMAP ]

[ V 0.1 ] Core Application & Basic Editor:
        * Platform:
            * SDL3 standalone application for MS Windows
        * Engine:
            * Event's with data pool (linked list of data atoms for RT safe transmutation)
        * Basic Functionality:
            * Save, Load, and Save-as for patches
            * Basic undo/redo support
        * Editing Essentials:
            * Simple set of objects
            * Copy/Paste functionality (using JSON as the interchange format)
        * User Interface:
            * Theme support
              Initial Plugin View
        * I/O Capabilities:
            * MIDI in/out
            * Audio in/out
            * Multichannel Audio in/out

[ V 0.2 ]
        * Make cross-platform: macOS, Linux, Windows

[ V 0.3 ]
        * CLAP plugin port
        
[ V 0.4 ]
        * Type in object names in canvas editor
        
[ v 0.5 ]
        * Sub-patches
        * Patch Abstractions
        
[ V 0.6 ]
        * Tabbed editor (Load multiple patches) - not split-view

[ V 0.7 ]
        * Compiled Abstractions
        
[ V 0.8 ]
        * Support more plugin formats via CLAP wrapper
        
[ V 0.9 ]
        * Refine plugin support (parameters etc)




[UI TODO]

* [DONE] correctly delete - make a system to have a focused component, currently using the clicked component (which is not the same)
* [DONE] scale / position canvas - make a way for the canvas to have a viewport - per component scaling
*        scrollbars - needed for canavs and side panels etc
* [DONE] Icons - simple icons to start with
* [DONE] Load patch etc
*        Desktop scale etc
*        Text entry (for object/nodes mainly)
*        Selected connections via lasso
*        Select multiple objects/connections with shift-click
* [DONE] Basic focus system (We need to think more about this - 
         what it means to gain focus? We still need to hover scroll components- but they wont have focus??)
         -- MAYBE?? 
         * We use a temporary focus system.
         * When there is a scroll event, we look at the current component's ancestors, and find who wants focus
         * If we don't find who wants focus we do nothing.
         * We also have an assignable focus system, which is what happens on mouse down - or when components request it.


[DSP TODO]
*        Improve oscillator for Triangle and Square. We need to use eblep (but with custom setup for each waveform)  
*        Square oscillator needs to be able to change duty cycle, need to allow for this... wavetables?



NOTES

[UI]
make delete work from a key pressed listener on the canvas itself

[ICONS]
https://fluenticons.co/outlined/

[BUILD]

adding WIN32 will make the app build without terminal (for debugging)
# PlugPatchStandalone target - use WIN32 to hide terminal
add_executable(PlugPatchStandalone PlugPatchStandalone.cpp ${GRAPH_SOURCES} ${UI_SOURCES})






WORK LOG:



[issue]
Use glaze for json handling (which will allow compile time etc)
[comments]
Explored using glaze, however currently glaze takes 2x as long to reflect json std::string to usable data
I may be doing something wrong, investigate further. Glaze has been left in system to easily hookup
----------

[issue]
Currently we can only load one patch at a time, allow loading multiple patches.
Assign each running patch the filename string that it came from

----------
[issue]

Added ID recycling, but now object ID system does not work if the generated ID is connected from a previous ID.

eg:
0 [metro]
1 [cnt]
2 [if]
3 [if]
4 [if]
5 [if]
6 [osc]
7 [osc]
8 [osc]
9 [osc]
11 [env]
12 [env]
13 [env]
14 [aout]
10 [osc] <-- unreachable
15 [osc]

[fix]
Make objectIDMap[finalID] equal the generated node ID

