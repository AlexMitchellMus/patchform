Project Goals

* Sample accurate events
* Similar node logic to Puredata
* Separation of audio / graph maintenance
* No clicking ever, for live patching etc
* Close to zero allocation on audio thread
* Simple / logical, and expressive file format (using json)
* Dynamic loading of compiled externals
* Sub-patches
* Multi-patch definitions (define multiple patches in same file, reuse the patch locally)
* Lightweight audio execution (no allocation in audio loop)
* Modern c++, make library as simple as possible.
* Set values for nodes from external controls (UI etc)

[UI TODO]

* [DONE] correctly delete - make a system to have a focused component, currently using the clicked component (which is not the same)
*        scale / position canvas - make a way for the canvas to have a viewport - per component scaling
*        scrollbars - needed for canavs and side panels etc
* [DONE] Icons - simple icons to start with
*        Load patch etc
*        Desktop scale etc
*        Text entry (for object/nodes mainly)

NOTES

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

