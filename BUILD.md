# Build Instructions

## Prerequisites

### Common
- CMake 3.10 or higher
- C++17 compatible compiler
- OBS Studio development files

### Linux (Ubuntu/Debian)
```bash
sudo apt-get install build-essential cmake git
sudo apt-get install libobs-dev libcurl4-openssl-dev libcjson-dev libwebsockets-dev
```

### macOS
```bash
brew install cmake libcurl cjson libwebsockets
```

Download OBS Studio source or install OBS Studio to get the development headers.

### Windows
- Visual Studio 2019 or newer with C++ support
- CMake
- vcpkg (recommended) or manual dependency installation

## Building

### Linux
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### macOS
```bash
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/path/to/obs ..
make -j$(sysctl -n hw.ncpu)
sudo make install
```

### Windows
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
cmake --install .
```

## Installation

The plugin will be installed to:
- **Linux**: `/usr/lib/obs-plugins/obs-streamlabels/`
- **macOS**: `/Applications/OBS.app/Contents/PlugIns/obs-streamlabels/`
- **Windows**: `C:\Program Files\obs-studio\obs-plugins\obs-streamlabels\`

## Troubleshooting

### Linux: "libobs-dev not found"
Install OBS Studio development headers from your distro's repository or build OBS from source.

### macOS: "OBS not found"
Set the CMAKE_PREFIX_PATH to point to your OBS installation:
```bash
cmake -DCMAKE_PREFIX_PATH=/Applications/OBS.app/Contents/ ..
```

### Windows: "cJSON not found"
Install dependencies via vcpkg:
```bash
vcpkg install curl:x64-windows cjson:x64-windows libwebsockets:x64-windows
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake
```
