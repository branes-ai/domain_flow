# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added

- **Documentation site** (`docs-site/`): Astro + Starlight site published to GitHub
  Pages at https://branes-ai.github.io/domain_flow/ via `.github/workflows/docs.yml`.
  Content is synced from the repo's `docs/` tree by `docs-site/sync-content.mjs`
  (sections: getting started, architecture, SURE simulator, theory, changelog),
  with KaTeX math rendering and a landing page at `docs/site/index.mdx`.
- **SURE simulator** (`include/dfa/sim/`, PR #3): standalone header-only functional
  simulator for Systems of Uniform/Affine Recurrence Equations — numeric evaluation
  with boundary/operand semantics, free (ASAP) schedule derivation, schedule
  legality checking (`tau.theta >= 1` with violation reports), memory-cardinality
  (peak live values) analysis, eviction-based execution, and a `.dfg` import path.
  Includes the `dfactl` CLI (`sim/`) with built-in `matmul`, `matvec`, and `qr`
  specs, plus tests under `src/dfa/tests/sim/`.
- **SURE simulator documentation** (`docs/sure-simulator.md`): why/what/how guide
  with worked `dfactl` examples (legal vs illegal schedules, stage offsets,
  heterogeneous-rank SAREs, `.dfg` import) and a spec-authoring walkthrough.
- **CLAUDE.md** (PR #5): guidance file for AI-assisted development with the
  verified build/test workflow (preset scheme, `compile_all` test naming,
  single-test invocation), code layout, and repo conventions.

### Changed

- **CI overhaul** (issue #7, PR #9): docs-only changes (`**.md`, `docs/**`,
  `docs-site/**`) no longer trigger the LLVM build matrix; a weekly scheduled
  run on main keeps the shared LLVM cache warm inside GitHub's 7-day eviction
  window; `restore-keys` + an install-tree probe (`mlir-tblgen` + `clang`) let
  prefix-restored caches skip the ~4h LLVM build (validated: warm matrix jobs
  complete in ~2-4 min); the cache key now fingerprints the real configure
  flags via the new `llvm-build-config.txt`; concurrency group auto-cancels
  superseded PR runs while protecting main/scheduled cache builds. Issue #10
  tracks the remaining cold-cache duplicate build across ubuntu compiler legs.

### Fixed

- **SURE simulator review hardening** (PR #3, 18 CodeRabbit findings): fail-fast
  validation in `Domain` (axis bounds, constraint dimensions), `AffineDependency`
  (ragged matrices), and `RecurrenceSystem` (duplicate equation names); schedule
  rank mismatches now throw instead of silently truncating (could mask illegal
  schedules); dependency-cycle detection in `eval()`/`computeFreeSchedule()`;
  drained outputs no longer counted as resident in the memory analysis; `.dfg`
  importer validates unary/binary operand shapes with per-axis broadcast
  compatibility and fails fast on multi-output producers; `dfactl --quiet` now
  works with `--dfg`; sim tests reuse the canonical spec builders.
- **Build documentation** (issue #4, PR #5): ARCHITECTURE.md, README.md, and
  SETUP.md referenced nonexistent CMake options (`DOMAINFLOW_ENABLE_TESTS`,
  `DOMAINFLOW_BUILD_TESTS`); corrected to the real `BUILD_TESTING` /
  `DOMAINFLOW_BUILD_TESTING` with accurate defaults.

- **Compiler warnings cleanup**: Eliminated all 6,720 compiler warnings with `-Wall -Wextra` flags
  - Added `default:` cases to switch statements in `domain_flow_node.hpp` and `domain_of_computation.hpp` (4,545 warnings)
  - Fixed unused variable warnings with `[[maybe_unused]]` attributes across multiple headers (1,274 warnings)
  - Fixed signed/unsigned comparison warnings by using `size_t` for loop indices (567 warnings)
  - Fixed unused parameter warnings with `[[maybe_unused]]` attributes (319 warnings)
  - Removed unused local typedef declarations from test files (11 warnings)
  - Added missing struct field initializers in `energy_estimator.hpp` and `energy_estimator.cpp` (4 warnings)

- **Bug fix in `domain_flow_edge.hpp`**: Constructor now properly initializes `srcSlot` and `dstSlot` member variables from parameters (previously ignored)
