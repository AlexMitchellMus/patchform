# Patchform

**Patchform** is a real-time audio node editor written in C++.

---

## ✨ Features

- ⚡ **Real-Time DSP Engine** – Lock-free, allocation-free during audio processing.
- 🎛 **Node-Based System** – Create graphs with audio, data, and event connections.
- 🧠 **Static Node Definition** – Nodes self-register with minimal boilerplate.
- 🔀 **Multi-Graph Swap** – Supports atomic swapping of full graphs with double buffering.
- 🖱️ **Custom UI Toolkit** – Built with NanoVG.
- 🔌 **PortAudio / RtMidi** – Cross-platform audio and MIDI I/O (coming soon).
- 🧩 **Modular Plugin SDK** – Designed to support DLL-style plugin modules (coming soon).

---

## 🛠️ Dependencies

- SDL3
- PortAudio
- RtMidi
- NanoVG
- simde (SIMD math library)
- moodycamel::ConcurrentQueue

---

## 📦 Packaging

### Windows

**NSIS Required**

To package the application for Windows using the provided build system:

1. Install **NSIS (Nullsoft Scriptable Install System)**  
   → https://nsis.sourceforge.io/

2. Ensure `makensis` is available in your system's `PATH`.

3. Build in **Release** mode and set `PackageInstaller` as the CMake target.

The installer will be generated at:  
`/installer/windows/PatchformStandaloneInstaller.exe`

> ⚠️ Packaging will fail if NSIS is not installed or if you're building in Debug mode.

---

## 📄 License

Patchform uses a **dual-license** model:

- **GPL3** for open-source projects
- **Commercial license** available for proprietary integrations

## 🤝 Contributions

Patchform is currently in active internal development.  
External contributions are not being accepted at this time — but stay tuned!