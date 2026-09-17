# Installation

## Prerequisites

- **C++20 compatible compiler** (GCC 11+, Clang 13+, or MSVC)
- **CMake** (>= 3.20)
- **Development libraries**: OpenGL, X11, pthread

On Debian/Ubuntu-based systems:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libgl1-mesa-dev libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev
```

On Fedora/RHEL-based systems:
```bash
sudo dnf install -y gcc-c++ cmake mesa-libGL-devel libX11-devel libXcursor-devel libXinerama-devel libXrandr-devel libXi-devel
```

On Arch Linux:
```bash
sudo pacman -S --needed base-devel cmake libgl xorg-server-devel
```

---

## Clone and Build

1. **Clone the repository with submodules:**
   ```bash
   git clone --recurse-submodules https://github.com/aaronma37/yazpads_warlock_sim.git
   cd yazpads_warlock_sim
   ```
   *(If already cloned without `--recurse-submodules`, initialize them with `git submodule update --init --recursive`)*

2. **Build Raylib static library (if not already built):**
   ```bash
   cd third_party/raylib/src
   make PLATFORM=PLATFORM_DESKTOP
   cd ../../..
   ```

3. **Configure and compile with CMake:**
   ```bash
   mkdir -p build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build . -j$(nproc)
   ```

The compiled binary will be placed in the `bin/` directory:
- `bin/warlock_sim` (Main executable)
- `bin/sim_tests` (Unit tests)
