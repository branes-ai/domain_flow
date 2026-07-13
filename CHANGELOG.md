# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added

- **SURE linear-algebra operator catalog** (epic #21): a catalog of executable
  SURE derivations of the key linear-algebra operators (BLAS L1/L2/L3,
  factorizations, solvers), each shipping the established triple — a `.sure`
  spec, a regression test, and a docs-site Theory page.
  - **`axpy`** (issue #22): the first entry — BLAS-1 `y := alpha*x + y` as a
    two-cell systolic map (`docs/SURE/axpy.sure`, `docs/SURE/axpy.md`, CTest
    `test_axpy_sure`, `dfactl_sure_axpy`). The fully-parallel vector map rides
    the length-N result on the `i` axis and injects `x` on a `j = -1` halo so
    the scaled contribution is added once as `y` streams to the terminal face.
    All three operands are uniform flows — in particular the scalar `alpha` is
    **projected** onto the `i = -1` edge and pipelined across the lanes
    (`a(i,j) = a(i-1,j)`) rather than baked into an equation body as a broadcast
    constant (which would be an affine, non-uniform dependence). Verified
    `R = alpha*X + Y` with derived face normals and free/linear legality.
    `SURE_DOCS_DIR` is now directory-scoped in the sim tests so each new
    `<op>_sure.cpp` is a drop-in.
- **SURE DSL: executable docs/SURE notation** (issue #15, PR #16; issue #17,
  PR #18): a header-only text front-end (`include/dfa/sim/sure_parser.hpp`)
  parses the `system((i,j,k) | constraints) { equations }` notation used in
  the theory documents into executable `RecurrenceSystem`s, wired into
  `dfactl --sure <file.sure>` with the existing `--schedule`/`--tau`/`--quiet`
  flags. A same-day design review (issue #17) replaced the v1
  boundary/table-input/projected-output statements with **v2 symmetric
  input/output confluence declarations**: equality-pinned face regions in the
  domain's own coordinates (extent from inequalities, location from the
  equality) with the orientation *derived* as the domain's outward normal,
  validated as a true supporting hyperplane. Machine-checked contracts, all
  with line-numbered diagnostics: tap-image coverage over declared input
  faces (face-dispatched boundaries — no silent data fabrication), face
  well-formedness, flux consistency (`tau.n < 0` influx / `> 0` outflux,
  revalidated for CLI `--tau` overrides via `validateSureFlux`), element
  range, and tensor/data binding.
- **Executable theory documents** (PR #19): all three `docs/SURE/` documents
  migrated to the v2 DSL with runnable kernels — `matmul.sure` (legality
  demos), `qr.sure` (Modified Gram-Schmidt uniformized onto one shared
  triangular domain: reductions as first-class variables, `q` embedded as a
  diagonal-tapped normalized flow, `R` leaving through two oriented faces
  including the non-axis-aligned `k = j` diagonal; verified `R` exact,
  `Q^T*Q = I` and `Q*R = A` to ~1e-15), and `conv2d.sure` (image as a
  value-preserving anti-diagonal flow seeded from two disjoint halo faces,
  padding as explicit data; verified against a direct correlation reference).
  QR_decomposition.md and conv2d.md rewritten around the uniformization
  decisions; the sim suite grew to 73 tests.
- **Fused MATMUL output-face epilogue** (issue #1, PR #13): a `MATMUL` node can
  declare a pointwise activation via `attribute["activation"]` (e.g. `"relu"`),
  recorded as an epilogue on the terminal `k = K-1` output-face `Confluence`;
  bias enters via the existing 3rd operand `Cin`. `Confluence` gained an
  optional epilogue field, MATMUL elaboration now populates the input/output
  face confluences (previously constructed and discarded), and the simulator
  importer executes the fused form `n(i,j) = act(c(i,j,K-1) + Cin(i,j))`.
  Attributes already round-trip through `.dfg` serialization.
- **`FUSED_MATMUL_BIAS_ACT` operator** (issue #2, PR #14): dedicated IR operator
  making the fused `Y = activation(A*B + bias)` semantics explicit — a single
  `(i,j,k)` domain where, unlike 3-input MATMUL (Cin seeds the accumulator at
  `k = 0`), the bias confluence sits on the *terminal* face where the epilogue
  executes. Shares a new `buildMatmulHull()` helper, the MATMUL constraint-set
  case, and the simulator lowering path. Coexists with the attribute form:
  attribute for MLIR-imported graphs, dedicated operator for explicit fused-IR
  construction (KPU simulator path). Verified wavefronts under output-stationary
  `tau = [1,1,1]`.
- **Documentation site** (`docs-site/`): Astro + Starlight site published to GitHub
  Pages at https://branes-ai.github.io/domain_flow/ via `.github/workflows/docs.yml`.
  Content is synced from the repo's `docs/` tree by `docs-site/sync-content.mjs`
  (sections: getting started, architecture, SURE simulator, theory, changelog),
  with KaTeX math rendering and a landing page at `docs/site/index.mdx`.
- **SURE simulator** (`include/dfa/sim/`, PR #3): standalone header-only functional
  simulator for Systems of Uniform/Affine Recurrence Equations — numeric evaluation
  with boundary/operand semantics, free schedule derivation, schedule
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

- **Terminology: drop the "ASAP" gloss on the free schedule.** The *free
  schedule* is the long-established (since the 1960s) term for the
  unencumbered, data-flow-earliest execution order; the parenthetical "(ASAP)"
  was inaccurate and has been removed from all docs and code (simulator
  comments, `dfactl` help/output strings, sim-test comments, and the
  architecture/simulator/QR theory docs).
- **CI overhaul** (issue #7, PR #9): docs-only changes (`**.md`, `docs/**`,
  `docs-site/**`) no longer trigger the LLVM build matrix; a weekly scheduled
  run on main keeps the shared LLVM cache warm inside GitHub's 7-day eviction
  window; `restore-keys` + an install-tree probe (`mlir-tblgen` + `clang`) let
  prefix-restored caches skip the ~4h LLVM build (validated: warm matrix jobs
  complete in ~2-4 min); the cache key now fingerprints the real configure
  flags via the new `llvm-build-config.txt`; concurrency group auto-cancels
  superseded PR runs while protecting main/scheduled cache builds. Issue #10
  tracks the remaining cold-cache duplicate build across ubuntu compiler legs.
- **CI: LLVM built once per OS** (issue #10, PR #12): a dedicated `llvm` job now
  owns the cache build/save per OS; the compiler matrix depends on it and does a
  read-only `actions/cache/restore` with a fail-fast verify, eliminating the
  cold-cache race where the ubuntu gcc and clang legs each built LLVM (~4h x 2).
  All cache steps upgraded to `actions/cache@v5`; workflow runs with
  least-privilege `permissions: contents: read`.

### Security

- **docs-site Dependabot alerts resolved** (PR #11): upgraded astro 5.18 → 7.0.6
  and @astrojs/starlight 0.34 → 0.41.3, clearing all six open alerts (2 high:
  Host-header SSRF, reflected XSS via slot names; plus XSS mediums and an
  esbuild dev-server low, now at 0.28.1). `npm audit`: 0 vulnerabilities.
  One migration: Starlight 0.39's sidebar `items` syntax.

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
