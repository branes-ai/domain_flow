# Domain Flow Repo Software Architecture

This file provides guidance when working with code in this repository.

## Project Overview

Domain Flow Architecture (DFA) is a header-only C++ library for building parallelizing compilers that analyze and optimize DL (Deep Learning) graphs. The project implements tools to read and analyze MLIR bytecode, finding efficient schedules and spatial reductions for executing DL graphs on various hardware configurations.

### Core Concept

The architecture represents DL graphs as **domain flow graphs** - chains of operators where each operator:
- Is defined by a System of Uniform or Affine Recurrence Equations (SURE or SARE)
- Has a domain of computation derived from tensor operands
- Executes in hypothesized multi-dimensional data paths

Unlike MLIR's linalg/affine dialects, which represent loop nests and memory views, the DFA dialect represents operators and domain flows with the goal of finding spatial reductions that avoid resource contention.

## Build System

### Prerequisites

- CMake 3.28+
- C++20 compiler (GCC 10+, Clang 12+, MSVC 2022+)
- vcpkg (Windows only, for dependency management)
- LLVM/MLIR (optional, only for MLIR tools)

**Platform notes:**
- Linux/macOS: Uses native system packages (apt, dnf, brew)
- Windows: Uses vcpkg for C++ dependencies

### CMake Configuration Strategy

The project uses **CMake Presets** with platform-specific templates:

1. **CMakePresets.json** (committed) - Shared base presets, platform-agnostic
2. **CMakeUserPresets.json.Linux.template** (committed) - Linux configuration template (no vcpkg)
3. **CMakeUserPresets.json.Windows.template** (committed) - Windows configuration template (with vcpkg)
4. **CMakeUserPresets.json** (gitignored) - Your personal configuration

#### Initial Setup - Linux

```bash
# 1. Copy the Linux template
cp CMakeUserPresets.json.Linux.template CMakeUserPresets.json

# 2. Edit to set your LLVM path (if using MLIR tools)
#    Linux uses native system packages, no vcpkg needed
```

#### Initial Setup - Windows

```powershell
# 1. Setup vcpkg (Windows only)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 2. Copy the Windows template
cp CMakeUserPresets.json.Windows.template CMakeUserPresets.json

# 3. Edit to set your paths:
#    - VCPKG_ROOT: C:/path/to/vcpkg
#    - LLVM_PROJECT_ROOT: C:/path/to/llvm-project (if using MLIR)
```

See `SETUP.md` for complete platform-specific setup instructions.

### Available Base Presets

Inherit from these in CMakeUserPresets.json:
- `Ninja-Debug` / `Ninja-Release`: Ninja generator builds (all platforms)
- `VS17-Debug` / `VS17-Release`: Visual Studio 2022 builds (Windows)

Platform-specific requirements:
- **Linux**: No vcpkg, uses native system packages
- **Windows**: Uses vcpkg for dependencies (add `CMAKE_TOOLCHAIN_FILE` in user presets)
- **LLVM/MLIR**: Set `LLVM_PROJECT_ROOT` environment variable in user presets

### Build Options

Configure with CMake options (all default to OFF unless noted):

```bash
# Core options
-DBUILD_TESTING=ON                    # CMake standard; default for DOMAINFLOW_BUILD_TESTING when root
-DDOMAINFLOW_BUILD_TESTING=ON         # Build and register tests (default: BUILD_TESTING as root, OFF as subproject)
-DDOMAINFLOW_VERBOSE_TESTS=ON         # Print all test output

# Feature toggles
-DDOMAINFLOW_TOOLS=ON                 # Enable dfg/rdg tools
-DDOMAINFLOW_POLYHEDRAL=ON            # Enable polyhedral tools (default: ON)
-DDOMAINFLOW_MLIR_TOOLS=ON            # Enable MLIR integration (requires LLVM)
-DDOMAINFLOW_DSE=ON                   # Enable design space exploration tools
-DDOMAINFLOW_VISUALIZATION=ON         # Enable visualization tools
-DDOMAINFLOW_MATPLOT_TOOLS=ON         # Enable matplotlib plotting
-DDOMAINFLOW_DATABASE_TOOLS=ON        # Enable database tools
```

### Build Commands

After setting up CMakeUserPresets.json:

```bash
# Configure using your user preset (creates build/user-ninja-release/)
cmake --preset user-ninja-release

# Build all targets
cmake --build build/user-ninja-release

# Run tests
ctest --test-dir build/user-ninja-release

# Or build with options
cmake --preset user-ninja-release -DDOMAINFLOW_TOOLS=ON -DDOMAINFLOW_DSE=ON
cmake --build build/user-ninja-release
```

**Important:** The build directory name matches your preset name (e.g., `build/user-ninja-release/`), not the base preset name.

#### Cleaning Build

If you encounter CMake cache issues:
```bash
rm -rf build/user-ninja-release
cmake --preset user-ninja-release
```

### Testing

Tests are organized by component:
- `src/json/tests/` - JSON library tests
- `src/graph/tests/` - Base graph library tests
- `src/dfa/tests/` - Domain flow architecture tests
- `src/dfa/tests/sim/` - SURE simulator tests (functional simulation, schedule legality, .dfg import)

To run a single test executable:
```bash
./build/Ninja-Release/bin/dfa_test_name
```

## Architecture

### Header-Only Library Structure

All core functionality is header-only in `include/`:
- `include/dfa/` - Domain flow architecture core
- `include/graph/` - Base graph data structures
- `include/energy/` - Energy modeling and estimation
- `include/util/` - Utilities

The `src/dfa/` directory contains only legacy skeleton CMakeLists files (as of 2025-03-29).

### Key Components

#### Domain Flow Graph (`include/dfa/domain_flow_graph.hpp`)
- Built on `sw::graph::directed_graph<DomainFlowNode, DomainFlowEdge>`
- Tracks source and sink nodes for domain flow
- Supports node depth assignment and subgraph extraction
- Key methods: `addNode()`, `addEdge()`, `assignNodeDepths()`, `distributeConstants()`

#### Domain of Computation (`include/dfa/domain_of_computation.hpp`)
- Represents the index space over which an operator computes
- Defined by constraint sets (affine inequalities)
- Used for dependency analysis and scheduling

#### Core Abstractions
- **Operators** (`domain_flow_operator.hpp`): Computation nodes defined by affine recurrence equations
- **Index Spaces** (`index_space.hpp`): Multi-dimensional iteration domains
- **Schedules** (`schedule.hpp`): Mappings from computation space to time
- **Wavefronts** (`wavefront.hpp`): Parallel execution fronts
- **Transformations** (`transformation.hpp`): Affine loop transformations

#### SURE Simulator (`include/dfa/sim/`)
A standalone, header-only functional simulator for Systems of Uniform/Affine
Recurrence Equations: numeric evaluation with boundary/operand semantics,
free (ASAP) schedule derivation, schedule legality checking (`tau.theta >= 1`),
memory-cardinality (peak live values) analysis, eviction-based execution, and a
`.dfg` import path that lowers DomainFlowGraphs into runnable recurrence systems.
See `docs/sure-simulator.md` for the why/what/how with worked examples.

### Tools

#### dfg tools (`tools/dfg/`)
Domain flow graph analysis and manipulation:
- `analyzer.cpp` - Graph analysis
- `domain_flow.cpp` - DFG construction
- `wavefront.cpp` - Wavefront extraction
- `pipeline_alignment.cpp` - Pipeline scheduling
- `domain_confluence.cpp` - Domain merging analysis

#### dfactl (`sim/`)
The SURE simulator CLI (built with `DOMAINFLOW_TOOLS=ON`): runs built-in recurrence
specs (`matmul`, `matvec`, `qr`) or imported `.dfg` graphs under free or linear
schedules, and reports legality and memory cardinality. See `docs/sure-simulator.md`.

#### dse tools (`tools/dse/`)
Design space exploration for finding optimal hardware mappings.

#### opt tools (`tools/opt/`)
MLIR optimization pass tools. Requires LLVM/MLIR:
- `dfa-opt`: MLIR optimizer with DFA passes
- Links against MLIR dialects: TOSA, Linalg, Tensor, Func, etc.

#### import tools (`tools/import/`)
MLIR import utilities for converting MLIR bytecode to DFG.

### MLIR Integration

When `DOMAINFLOW_MLIR_TOOLS=ON`, the project integrates with LLVM/MLIR:
- `include/dfa/mlir/` - MLIR dialect interfaces (TOSA, Torch, StableHLO)
- `include/dfa/dfa_mlir.hpp` - MLIR-DFA bridge
- Requires `MLIR_DIR` pointing to MLIR CMake config

### Workloads

`workloads/` contains example DFG generators:
- `workloads/dnn/` - Deep neural network workloads
- `workloads/nla/` - Numerical linear algebra
- `workloads/dsp/` - Digital signal processing
- `workloads/cnn/` - Convolutional neural networks
- `workloads/ctl/` - Control flow examples
- `workloads/dfa/` - Domain flow specific examples

## Development Workflow

### Adding New DFG Operators

1. Define operator in `include/dfa/domain_flow_operator.hpp`
2. Implement domain computation in `include/dfa/domain_of_computation.hpp`
3. Add test in `src/dfa/tests/`
4. Optionally add workload example in `workloads/`

### Working with Tests

Each `.cpp` file in test directories becomes a separate executable via the `compile_all` macro. Tests are automatically discovered and registered with CTest when `DOMAINFLOW_BUILD_TESTING=ON`.

### Debugging

The `DOMAINFLOW_CMAKE_TRACE` option enables verbose CMake variable tracing during configuration.

## Historical Context

Domain Flow Architecture originates from E. Theodore Omtzigt's Yale PhD dissertation (1990-1993) "Domain Flow and Streaming Architectures: A Paradigm for Efficient Parallel Computation". The approach emphasizes:
- Minimizing data movement (primary energy cost)
- Maximizing concurrency and data locality
- Streaming execution driven by data availability
- Static scheduling for efficient resource usage

See `docs/domain-flow-history.md` for detailed comparison with systolic arrays, spatial dataflow accelerators, CGRAs, and neuromorphic architectures.

## Key Design Principles

1. **Data Movement Minimization**: Domain flows are designed to maximize on-chip data reuse
2. **Static Scheduling**: Compile-time analysis determines execution order
3. **Spatial Mapping**: Algorithms map to hardware fabric topologies
4. **Domain Decomposition**: Problems structured by their computational domains
5. **Header-Only Design**: Ease of integration without library dependencies
