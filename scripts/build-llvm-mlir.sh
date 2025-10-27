#!/bin/bash
# Script to build LLVM/MLIR 20.x for testing Domain Flow MLIR tools
# This mirrors the CI build configuration

set -e

LLVM_BRANCH="release/20.x"
LLVM_SRC_DIR="${HOME}/dev/clones/llvm-project"
LLVM_BUILD_DIR="${HOME}/dev/builds/llvm-20x"
LLVM_INSTALL_DIR="${HOME}/dev/installs/llvm-20x"

echo "Building LLVM/MLIR ${LLVM_BRANCH}"
echo "Source:  ${LLVM_SRC_DIR}"
echo "Build:   ${LLVM_BUILD_DIR}"
echo "Install: ${LLVM_INSTALL_DIR}"

# Clone LLVM if not already present
if [ ! -d "${LLVM_SRC_DIR}" ]; then
    echo "Cloning LLVM..."
    git clone https://github.com/llvm/llvm-project.git "${LLVM_SRC_DIR}" \
        --branch "${LLVM_BRANCH}" --depth 1
else
    echo "LLVM source already exists at ${LLVM_SRC_DIR}"
    echo "To update: cd ${LLVM_SRC_DIR} && git pull"
fi

# Create build directory
mkdir -p "${LLVM_BUILD_DIR}"
cd "${LLVM_BUILD_DIR}"

# Configure LLVM (matches CI configuration)
echo "Configuring LLVM..."
cmake "${LLVM_SRC_DIR}/llvm" \
    -GNinja \
    -DLLVM_ENABLE_PROJECTS="mlir;clang" \
    -DLLVM_TARGETS_TO_BUILD="host" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DCMAKE_INSTALL_PREFIX="${LLVM_INSTALL_DIR}"

# Build and install
echo "Building LLVM (this will take a while - 30-60 minutes)..."
cmake --build . --target install -j$(nproc)

echo ""
echo "✅ LLVM/MLIR build complete!"
echo ""
echo "Installation directory: ${LLVM_INSTALL_DIR}"
echo ""
echo "To use with Domain Flow, add to your CMakeUserPresets.json:"
echo "  \"LLVM_PROJECT_ROOT\": \"${LLVM_SRC_DIR}\""
echo ""
echo "Then configure Domain Flow:"
echo "  cmake --preset user-ninja-release \\"
echo "    -DDOMAINFLOW_MLIR_TOOLS=ON \\"
echo "    -DLLVM_DIR=${LLVM_INSTALL_DIR}/lib/cmake/llvm \\"
echo "    -DMLIR_DIR=${LLVM_INSTALL_DIR}/lib/cmake/mlir"
