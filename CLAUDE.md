# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Domain Flow Architecture (DFA) is a header-only C++20 library for building parallelizing compilers that analyze DL graphs (via MLIR bytecode) to find efficient schedules and spatial mappings. DL graphs are represented as **domain flow graphs**: chains of operators, each defined by a System of Affine/Uniform Recurrence Equations (SARE/SURE) with a domain of computation derived from its tensor operands. Unlike MLIR's linalg/affine dialects (loop nests and memory views), the DFA representation models operators and domain flows to find spatial reductions that avoid resource contention.

See `ARCHITECTURE.md` for the full architecture guide (key components, MLIR integration, historical context) — it is the authoritative reference and should be kept in sync with structural changes.

## Build System

CMake presets with a two-layer scheme: `CMakePresets.json` (committed base presets: `Ninja-Debug`, `Ninja-Release`, `VS17-*`) and `CMakeUserPresets.json` (gitignored, personal). **First-time setup on Linux:**

```bash
cp CMakeUserPresets.json.Linux.template CMakeUserPresets.json
# edit LLVM_PROJECT_ROOT only if building MLIR tools
```

(Windows uses the `.Windows.template` and vcpkg; Linux uses native system packages, no vcpkg.)

### Build and test

```bash
cmake --preset user-ninja-release            # configure; build dir = build/<presetName>
cmake --build build/user-ninja-release       # build all targets
ctest --test-dir build/user-ninja-release    # run all tests
```

The build directory name matches the *user* preset name (e.g. `build/user-ninja-release/`), not the base preset. On cache trouble, delete the build directory and reconfigure.

The VS17 presets use a multi-config generator (build dir `build_msvc/<presetName>`), so pass the configuration explicitly there:

```bash
cmake --build build_msvc/user-vs17 --config Release
ctest --test-dir build_msvc/user-vs17 --build-config Release
```

### Running a single test

Every `.cpp` file in a `tests/` directory becomes its own executable via the `compile_all` macro (`cmake/domain_flow_helpers.cmake`), named `<prefix>_<filename>` — test dirs use prefix `test`, so `src/dfa/tests/matmul.cpp` → target and CTest name `test_matmul`.

```bash
ctest --test-dir build/user-ninja-release -R test_matmul   # via ctest
cmake --build build/user-ninja-release --target test_matmul  # rebuild just one
```

Executables can also be run directly from the build tree. (With VS17 presets, add `--build-config Release` / `--config Release` as above.)

### Feature options (all OFF by default; ninja presets turn TOOLS and DSE ON)

- `DOMAINFLOW_TOOLS` — dfg/rdg analysis tools
- `DOMAINFLOW_DSE` — design space exploration tools
- `DOMAINFLOW_MLIR_TOOLS` — MLIR integration (`dfa-opt`, importers); requires an LLVM/MLIR 20.x build (`./scripts/build-llvm-mlir.sh`, then set `LLVM_DIR`/`MLIR_DIR` in user presets)
- `DOMAINFLOW_POLYHEDRAL`, `DOMAINFLOW_VISUALIZATION`, `DOMAINFLOW_MATPLOT_TOOLS`, `DOMAINFLOW_DATABASE_TOOLS`
- `DOMAINFLOW_VERBOSE_TESTS` — print all test output; `DOMAINFLOW_CMAKE_TRACE` — verbose CMake variable tracing

## Code Layout

- `include/` — all core functionality is header-only here:
  - `include/dfa/` — DFA core: `domain_flow_graph.hpp` (built on `sw::graph::directed_graph<DomainFlowNode, DomainFlowEdge>`), `domain_of_computation.hpp` (constraint sets / index spaces), `domain_flow_operator.hpp`, `index_space.hpp`, `schedule.hpp`, `wavefront.hpp`, `transformation.hpp`
  - `include/dfa/mlir/` — MLIR dialect interfaces (TOSA, Torch, StableHLO); `dfa_mlir.hpp` is the MLIR–DFA bridge
  - `include/graph/` — base graph data structures; `include/energy/` — energy modeling; `include/util/` — utilities
- `src/{dfa,graph,json}/tests/` — the test suites (the `src/` non-test code is largely legacy skeleton)
- `tools/` — executables: `dfg/` (graph analysis, wavefronts, pipeline alignment), `dse/`, `opt/` (`dfa-opt`, needs MLIR), `import/` (MLIR → DFG), `rdg/`, `viz/`
- `workloads/` — example DFG generators by domain (`dnn/`, `nla/`, `dsp/`, `cnn/`, `ctl/`, `dfa/`); useful as usage examples of the API

## Adding a New DFG Operator

1. Define the operator in `include/dfa/domain_flow_operator.hpp`
2. Implement domain computation in `include/dfa/domain_of_computation.hpp`
3. Add a test in `src/dfa/tests/` (just drop in a `.cpp`; CMake auto-discovers it)
4. Optionally add a workload example in `workloads/`

## Conventions

- The codebase was recently brought to zero warnings under `-Wall -Wextra`; keep it that way (use `[[maybe_unused]]`, `size_t` loop indices, `default:` cases in switches over enums).
- Notable changes go in `CHANGELOG.md` (Keep a Changelog format, under `[Unreleased]`); session logs live in `docs/sessions/` (see the `/wrapup` skill).
- `docs/` contains extensive background material (MLIR/IREE architecture notes, SURE formalism, domain-flow history) — search there before researching externally.
