# Installation

## Prerequisites

### Linux
- **C++20 compiler** (GCC 11+, Clang 13+)
- **CMake** (>= 3.20)
- **Development libraries**: OpenGL, X11, pthread

```bash
# Debian / Ubuntu
sudo apt-get update && sudo apt-get install -y build-essential cmake libgl1-mesa-dev libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev

# Fedora / RHEL
sudo dnf install -y gcc-c++ cmake mesa-libGL-devel libX11-devel libXcursor-devel libXinerama-devel libXrandr-devel libXi-devel

# Arch Linux
sudo pacman -S --needed base-devel cmake libgl xorg-server-devel
```

### Windows
- **Visual Studio 2022** (with *Desktop development with C++*) or **MSYS2 / MinGW-w64** (GCC 11+)
- **CMake** (>= 3.20)
- **Git**

*(Alternatively, you can build and run seamlessly via **WSL2** using the Linux instructions).*

---

## Clone Repository

Clone the repository with submodules initialized:

```bash
git clone --recurse-submodules https://github.com/aaronma37/yazpads_warlock_sim.git
cd yazpads_warlock_sim
```

If already cloned without `--recurse-submodules`, initialize them:
```bash
git submodule update --init --recursive
```

---

## Build Instructions

### Linux / macOS / WSL2

1. **Build the Raylib static library:**
   ```bash
   cd third_party/raylib/src
   make PLATFORM=PLATFORM_DESKTOP
   cd ../../..
   ```

2. **Configure and compile with CMake:**
   ```bash
   mkdir -p build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build . -j$(nproc)
   ```

---

### Windows

#### Option A: Visual Studio / MSVC (Developer Command Prompt / PowerShell)

1. **Build Raylib with CMake:**
   ```cmd
   cd third_party\raylib
   mkdir build && cd build
   cmake -DPLATFORM=Desktop -DBUILD_EXAMPLES=OFF ..
   cmake --build . --config Release
   copy /Y raylib\Release\raylib.lib ..\src\raylib.lib
   cd ..\..\..
   ```

2. **Build the simulator:**
   ```cmd
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build . --config Release
   ```

#### Option B: MSYS2 (MinGW 64-bit)

```bash
# Inside MSYS2 MINGW64 shell:
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make git

cd third_party/raylib/src
make PLATFORM=PLATFORM_DESKTOP
cd ../../..

mkdir -p build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

---

## Binaries

Compiled binaries will be located in the `bin/` directory:
- `bin/warlock_sim` (or `bin/Release/warlock_sim.exe` on MSVC)
- `bin/sim_tests` (or `bin/Release/sim_tests.exe` on MSVC)
