# IREE GitHub actions

A GitHub Actions workflow to clone, build, and cache IREE instead of LLVM/MLIR. Here's the updated workflow:


```yaml
name: IREE Build on Multiple Platforms

on:
  push:
    branches: [ "main" ]
  pull_request:
    branches: [ "main" ]

jobs:
  build:
    runs-on: ${{ matrix.os }}

    strategy:
      # Set fail-fast to false to ensure that feedback is delivered for all matrix combinations
      fail-fast: false

      matrix:
        os: [ubuntu-latest, windows-latest]
        build_type: [Release]
        c_compiler: [gcc, clang, cl]
        include:
          - os: windows-latest
            c_compiler: cl
            cpp_compiler: cl
          - os: ubuntu-latest
            c_compiler: gcc
            cpp_compiler: g++
          - os: ubuntu-latest
            c_compiler: clang
            cpp_compiler: clang++
        exclude:
          - os: windows-latest
            c_compiler: gcc
          - os: windows-latest
            c_compiler: clang
          - os: ubuntu-latest
            c_compiler: cl

    steps:
    - uses: actions/checkout@v4
      with:
        submodules: recursive

    - name: Set up Python
      uses: actions/setup-python@v4
      with:
        python-version: '3.12'

    - name: Cache IREE Build
      id: cache-iree-build
      uses: actions/cache@v3
      with:
        path: |
          iree-install
          iree-build
        key: ${{ runner.os }}-iree-${{ hashFiles('**/CMakeLists.txt', '.git/modules/**/HEAD') }}
        restore-keys: |
          ${{ runner.os }}-iree-

    - name: Clone IREE (if not cached)
      if: steps.cache-iree-build.outputs.cache-hit != 'true'
      run: |
        git clone https://github.com/iree-org/iree.git --depth 1
        cd iree
        git submodule update --init --recursive

    - name: Setup build dependencies (Unix)
      if: runner.os == 'Linux'
      run: |
        sudo apt-get update
        sudo apt-get install -y \
          cmake \
          ninja-build \
          libsdl2-dev \
          python3-pip \
          python3-venv

    - name: Setup build dependencies (Windows)
      if: runner.os == 'Windows'
      run: |
        choco install cmake ninja
        pip install lit

    - name: Build IREE (Unix/macOS)
      if: runner.os == 'Linux'
      run: |
        mkdir -p iree-build
        cd iree-build
        cmake ../iree \
          -GNinja \
          -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
          -DCMAKE_C_COMPILER=${{ matrix.c_compiler }} \
          -DCMAKE_CXX_COMPILER=${{ matrix.cpp_compiler }} \
          -DCMAKE_INSTALL_PREFIX="${{ github.workspace }}/iree-install" \
          -DIREE_BUILD_TESTS=ON \
          -DIREE_BUILD_SAMPLES=OFF
        cmake --build . --target install -j$(nproc)

    - name: Build IREE (Windows)
      if: runner.os == 'Windows'
      shell: pwsh
      run: |
        mkdir iree-build
        cd iree-build
        cmake ..\iree `
          -GNinja `
          -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} `
          -DCMAKE_C_COMPILER=${{ matrix.c_compiler }} `
          -DCMAKE_CXX_COMPILER=${{ matrix.cpp_compiler }} `
          -DCMAKE_INSTALL_PREFIX="${{ github.workspace }}/iree-install" `
          -DIREE_BUILD_TESTS=ON `
          -DIREE_BUILD_SAMPLES=OFF
        cmake --build . --target install -j $env:NUMBER_OF_PROCESSORS

    - name: Setup environment
      run: |
        echo "${{ github.workspace }}/iree-install/bin" >> $GITHUB_PATH

    - name: Set reusable strings
      id: strings
      shell: bash
      run: |
        echo "build-output-dir=${{ github.workspace }}/build" >> "$GITHUB_OUTPUT"

    - name: Configure Project CMake
      run: >
        cmake -B ${{ steps.strings.outputs.build-output-dir }}
        -DCMAKE_CXX_COMPILER=${{ matrix.cpp_compiler }}
        -DCMAKE_C_COMPILER=${{ matrix.c_compiler }}
        -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}
        -DIREE_DIR=${{ github.workspace }}/iree-install/lib/cmake/iree
        -S ${{ github.workspace }}

    - name: Build Project
      run: cmake --build ${{ steps.strings.outputs.build-output-dir }} --config ${{ matrix.build_type }}

    - name: Test
      working-directory: ${{ steps.strings.outputs.build-output-dir }}
      run: ctest --build-config ${{ matrix.build_type }}
```

Key modifications from the original LLVM workflow:

1. Replaced LLVM/MLIR-specific clone and build steps with IREE-specific ones
2. Added submodule initialization for IREE
3. Included dependency installation steps for different platforms
4. Modified CMake configuration to use IREE-specific flags and paths
5. Adjusted caching strategy to include IREE build and install directories
6. Used more generic build configurations

Notable differences and considerations:
- The workflow now clones IREE and its submodules
- Added platform-specific dependency installation steps
- Configured IREE build with tests enabled and samples disabled
- Created cache for IREE build to speed up subsequent builds
- Maintained multi-platform and multi-compiler support

To use this workflow, you'll need to ensure your project's CMakeLists.txt can find and use the IREE installation. The `-DIREE_DIR` flag points to the CMake configuration files installed by IREE.


