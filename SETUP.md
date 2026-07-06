# Domain Flow Setup Guide

## Quick Start

### 1. Install Prerequisites

#### Linux/macOS
- **CMake** 3.28+
- **C++20 compiler** (GCC 10+, Clang 12+)
- **Ninja** build system (optional but recommended)
- System dependencies (installed via package manager)

```bash
# Ubuntu/Debian
sudo apt install cmake ninja-build g++

# Fedora/RHEL
sudo dnf install cmake ninja-build gcc-c++

# macOS (with Homebrew)
brew install cmake ninja
```

#### Windows
- **CMake** 3.28+
- **Visual Studio 2022** with C++ development tools OR **Ninja**
- **vcpkg** package manager

```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Set environment variable (PowerShell)
$env:VCPKG_ROOT = "C:\dev\vcpkg"
# Or set permanently via System Properties > Environment Variables
```

#### Optional (for MLIR tools, all platforms)
- **LLVM/MLIR** 17+ built from source

### 2. Configure CMake User Presets

**Choose the template for your platform:**

#### Linux (uses native system dependencies, no vcpkg)

```bash
cp CMakeUserPresets.json.Linux.template CMakeUserPresets.json
```

Edit `CMakeUserPresets.json` to set your LLVM path:

```json
{
  "configurePresets": [
    {
      "name": "user-ninja-release",
      "inherits": "Ninja-Release",
      "environment": {
        "LLVM_PROJECT_ROOT": "/home/yourname/dev/clones/llvm-project"
      }
    }
  ]
}
```

#### Windows (uses vcpkg for dependencies)

```powershell
cp CMakeUserPresets.json.Windows.template CMakeUserPresets.json
```

Edit `CMakeUserPresets.json` to set your local paths:

```json
{
  "configurePresets": [
    {
      "name": "user-vs17-release",
      "inherits": "VS17-Release",
      "environment": {
        "VCPKG_ROOT": "C:/dev/vcpkg",
        "LLVM_PROJECT_ROOT": "C:/dev/clones/llvm-project"
      },
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": {
          "type": "FILEPATH",
          "value": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        }
      }
    }
  ]
}
```

**Note:** `CMakeUserPresets.json` is gitignored and won't be committed. The platform-specific templates are checked in to preserve configuration knowledge.

### 3. Configure and Build

#### Linux - Basic Build (no MLIR)

```bash
# Configure (uses native system packages)
cmake --preset user-ninja-release

# Build
cmake --build build/Ninja-Release

# Run tests (if enabled)
ctest --test-dir build/Ninja-Release
```

#### Windows - Basic Build (with vcpkg)

```powershell
# Configure (uses vcpkg for dependencies)
cmake --preset user-vs17-release

# Build
cmake --build build_msvc/VS17-Release

# Or use Ninja on Windows
cmake --preset user-ninja-release
cmake --build build/Ninja-Release
```

#### All Platforms - With MLIR Tools

First, build LLVM/MLIR (Linux/macOS):

```bash
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir build && cd build

cmake -G Ninja ../llvm \
  -DLLVM_ENABLE_PROJECTS="mlir" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_TARGETS_TO_BUILD="X86" \
  -DLLVM_ENABLE_ASSERTIONS=ON

ninja
```

Windows (use `build_msvc` directory):
```powershell
cd llvm-project
mkdir build_msvc
cd build_msvc

cmake -G "Visual Studio 17 2022" ..\llvm `
  -DLLVM_ENABLE_PROJECTS="mlir" `
  -DCMAKE_BUILD_TYPE=Release `
  -DLLVM_TARGETS_TO_BUILD="X86"

cmake --build . --config Release
```

Then configure Domain Flow with MLIR:

```bash
# Ensure CMakeUserPresets.json has:
# "LLVM_PROJECT_ROOT": "/path/to/llvm-project"

# Configure with MLIR tools enabled
cmake --preset user-ninja-release -DDOMAINFLOW_MLIR_TOOLS=ON

# Build
cmake --build build/Ninja-Release
```

## Alternative: Environment Variables

Instead of using `CMakeUserPresets.json`, you can set environment variables:

```bash
# Linux/macOS
export VCPKG_ROOT=/path/to/vcpkg
export LLVM_PROJECT_ROOT=/path/to/llvm-project

# Then use the base presets
cmake --preset Ninja-Release

# Windows (PowerShell)
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
$env:LLVM_PROJECT_ROOT = "C:\path\to\llvm-project"

cmake --preset Ninja-Release
```

## Build Options

Control features with CMake options:

```bash
cmake --preset user-ninja-release \
  -DDOMAINFLOW_BUILD_TESTING=ON \
  -DDOMAINFLOW_TOOLS=ON \
  -DDOMAINFLOW_DSE=ON \
  -DDOMAINFLOW_MLIR_TOOLS=OFF
```

### Available Options

| Option | Default | Description |
|--------|---------|-------------|
| `DOMAINFLOW_BUILD_TESTING` | `BUILD_TESTING` (root) / OFF (subproject) | Build and register tests |
| `DOMAINFLOW_TOOLS` | OFF | Build dfg/rdg tools |
| `DOMAINFLOW_POLYHEDRAL` | ON | Build polyhedral tools |
| `DOMAINFLOW_MLIR_TOOLS` | OFF | Build MLIR integration (requires LLVM) |
| `DOMAINFLOW_DSE` | OFF | Build design space exploration tools |
| `DOMAINFLOW_VISUALIZATION` | OFF | Build visualization tools |
| `DOMAINFLOW_MATPLOT_TOOLS` | OFF | Build matplotlib integration |
| `DOMAINFLOW_DATABASE_TOOLS` | OFF | Build database tools |

## Troubleshooting

### "Could not find toolchain file: /scripts/buildsystems/vcpkg.cmake"

This happens when CMake cache has stale vcpkg configuration:

```bash
# Clean the build directory
rm -rf build/user-ninja-release

# Reconfigure
cmake --preset user-ninja-release
```

### "CMake was unable to find a build program corresponding to Ninja"

Install Ninja:
```bash
# Ubuntu/Debian
sudo apt install ninja-build

# Fedora/RHEL
sudo dnf install ninja-build

# macOS
brew install ninja
```

### vcpkg not found (Windows only)

Ensure `VCPKG_ROOT` environment variable is set:
```powershell
echo $env:VCPKG_ROOT  # Should print vcpkg path
```

### MLIR not found

If building with `DOMAINFLOW_MLIR_TOOLS=ON`:
1. Verify `LLVM_PROJECT_ROOT` is set in CMakeUserPresets.json
2. Ensure LLVM/MLIR is built
3. Pass `-DMLIR_DIR` explicitly if needed

### Preset not found

If you see "preset not found", ensure you've created `CMakeUserPresets.json` from the appropriate platform template.

### Compiler warnings about overflow

The warnings in `index_space.hpp` about overflow conversions are expected and harmless. They occur when using `double::infinity()` with integer types for sentinel values.

## Platform-Specific Notes

### Linux
- Use `Ninja-Debug` or `Ninja-Release` presets (or user variants)
- Ensure CMake 3.31+ is installed via system package manager or from source

### Windows
- Use `VS17-Debug` or `VS17-Release` presets for Visual Studio
- Or use `Ninja-Debug`/`Ninja-Release` with Ninja generator
- Install Visual Studio 2022 with C++ development tools

### macOS
- Use `Ninja-Debug` or `Ninja-Release` presets
- Install CMake via Homebrew: `brew install cmake`
- Ensure Xcode Command Line Tools are installed
