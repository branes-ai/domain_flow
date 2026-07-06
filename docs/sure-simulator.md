# The SURE Simulator

A functional simulator for Systems of Uniform/Affine Recurrence Equations (SURE/SARE),
with schedule legality checking, memory-cardinality analysis, and a `.dfg` import path.
It lives in `include/dfa/sim/` (header-only) with a CLI front end, `dfactl`, in `sim/`.

## Why

Domain flow analysis makes claims that deserve computational proof. When the compiler
proposes a linear schedule `t = tau.p + beta[var]` for a recurrence system, three
questions must be answerable *before* committing to a spatial mapping:

1. **Is the schedule legal?** Every dependency edge needs `tau.theta >= 1`
   (the producer must fire strictly before its consumer). A schedule that violates
   this on even one edge is not executable.
2. **What does the schedule cost in memory?** The number of simultaneously live
   values under a schedule is the number of memories the spatial fabric needs.
   Different legal schedules for the same system have different peaks: the free
   (ASAP) schedule maximizes parallelism but also the resident set; a linear
   wavefront schedule typically trades latency for a smaller footprint.
3. **Does the theory match execution?** An analytical peak is only trustworthy if
   an actual eviction-based execution of the schedule realizes the same footprint.

The simulator answers all three: it *executes* recurrence systems numerically
(verifying outputs against references), *checks* schedules against every dependency
edge, and *measures* liveness both analytically (birth/death sweep) and empirically
(running with eviction and comparing footprints). It is the test bench for the
scheduling side of the domain flow theory, on the path toward automatic schedule
and spatial-reduction search for imported DL graphs.

## What

The simulator consists of small, dependency-free headers under `include/dfa/sim/`
(they mirror their full-library twins — `Domain` ~ `DomainOfComputation`,
`AffineDependency` ~ `AffineMap<int>` — so they compile standalone):

| Header | Provides |
|---|---|
| `recurrence_system.hpp` | `Equation` (name, domain, taps, compute, boundary) and `RecurrenceSystem` |
| `domain.hpp` | `HalfSpace`, `Domain`: constraint sets with `isInside()` / `enumerate()` |
| `affine_dependency.hpp` | `AffineDependency`: the map `f(p) = A*p + b` from consumer to producer point |
| `schedule_strategy.hpp` | `ISchedule` with `LinearSchedule`, `PiecewiseLinearSchedule`, `ExplicitSchedule` |
| `legality.hpp` | `checkLegality()`: slack on every dependency edge, violation report |
| `sure_simulator.hpp` | `SureSimulator`: `eval()`, `computeFreeSchedule()`, `analyzeMemory()`, `run()` |
| `specs.hpp` | Built-in `Spec` builders: `matmul`, `matvec`, `qr` |
| `dfg_import.hpp` | Lower a `DomainFlowGraph` (`.dfg` file) into a runnable `RecurrenceSystem` |

Capabilities:

- **Functional evaluation** (`eval`): lazy, memoized evaluation of the recurrence.
  A tap that resolves outside its producer's domain is a *boundary* — that is how
  operand data and initial conditions enter the system. Input operands are simply
  equations with an empty domain, so they stream without occupying memory.
  Dependency cycles are detected and diagnosed instead of overflowing the stack.
- **Free (ASAP) schedule** (`computeFreeSchedule`): the data-flow earliest firing
  time of every value — minimum latency, maximum parallelism, widest wavefronts.
- **Legality checking** (`checkLegality`): validates `time(consumer) - time(producer) >= 1`
  on every dependency edge; reports edge count, minimum slack, and a capped sample
  of violations. Unresolved tap sources are counted rather than silently skipped.
- **Memory cardinality** (`analyzeMemory`): birth/death liveness sweep under a
  schedule; reports peak live values (total and per variable), latency, and work.
  Never-consumed values are *outputs*: they drain through a boundary face at birth
  and are not counted as resident.
- **Eviction-based execution** (`run`): executes in schedule order holding only
  live values, refuses illegal schedules up front with the full report, and returns
  the drained outputs. Its observed footprint must equal `analyzeMemory()`'s peak —
  the CLI and tests assert exactly that.
- **`.dfg` import** (`importDfgFile`): lowers each graph node to a recurrence
  variable over its result-tensor index space. MATMUL becomes the canonical 3D
  accumulation recurrence; elementwise unary/binary ops become broadcast-mapped
  equations; unsupported operators are reported and modeled as zero sources so the
  rest of the graph still elaborates.

## How

### Build and run

`dfactl` is built when `DOMAINFLOW_TOOLS=ON` (the default in the ninja presets):

```bash
cmake --preset user-ninja-release
cmake --build build/user-ninja-release --target dfactl
./build/user-ninja-release/sim/dfactl --list
```

```text
Available specs:
  matmul	C = C0 + A*B  (2x2, uniform recurrence)
  matvec	y = A*x  (2x3, instantaneous y<-xs dependency)
  qr	A = Q*R  (3x3 Modified Gram-Schmidt, SARE)
```

### Example 1: legal vs illegal schedules (matmul)

The matmul spec is the canonical systolic recurrence (see `docs/SURE/matmul.md`
for the derivation):

```text
system((i,j,k) | 0 <= i,j,k < N) {
    a(i,j,k) = a(i,j-1,k);                          // propagate A along +j
    b(i,j,k) = b(i-1,j,k);                          // propagate B along +i
    c(i,j,k) = c(i,j,k-1) + a(i,j-1,k)*b(i-1,j,k);  // accumulate along +k
}
```

Under the classic wavefront schedule `tau = [1,1,1]`:

```console
$ dfactl matmul --tau 1,1,1
spec: matmul  (C = C0 + A*B  (2x2, uniform recurrence))
C = C0 + A*B:
  C[0][0] = 29  (ref 29)
  C[0][1] = 42  (ref 42)
  C[1][0] = 73  (ref 73)
  C[1][1] = 90  (ref 90)

schedule: linear tau=[1,1,1]

Schedule legality: LEGAL  edges=20  minSlack(tau.theta)=1
Memory cardinality: peakLiveValues=9  latency=4  work=24
  per-variable peak live: a=3 b=3 c=3
  realized footprint (with eviction): 9

OK
```

Zeroing the `k` component breaks the accumulation dependency of `c`
(`theta = (0,0,1)`, so `tau.theta = 0`), and the simulator names the exact edges:

```console
$ dfactl matmul --tau 1,1,0
...
Schedule legality: ILLEGAL  edges=20  minSlack(tau.theta)=0  violations=4:
    c(0, 0, 1) reads c(0, 0, 0)  slack=0 (need >= 1)
    c(0, 1, 1) reads c(0, 1, 0)  slack=0 (need >= 1)
    c(1, 0, 1) reads c(1, 0, 0)  slack=0 (need >= 1)
    c(1, 1, 1) reads c(1, 1, 0)  slack=0 (need >= 1)
Schedule is illegal; skipping execution.

FAILED
```

Comparing the default free (ASAP) schedule with the linear one shows the
latency/memory trade-off — the free schedule is one step faster but needs a third
more memories (12 vs 9):

```console
$ dfactl matmul          # free (ASAP) schedule
Schedule legality: LEGAL  edges=20  minSlack(tau.theta)=1
Memory cardinality: peakLiveValues=12  latency=3  work=24
```

### Example 2: stage offsets (matvec)

In the matvec spec, `y` reads `xs` at the *same* index point, so a bare
`tau = [1,1]` has zero slack on that edge. A per-variable offset `beta[y] = 1`
(a pipeline stage delay) makes it legal — this is the spec's canonical schedule:

```console
$ dfactl matvec --schedule linear
schedule: linear tau=[1,1] beta{y=1 }

Schedule legality: LEGAL  edges=13  minSlack(tau.theta)=1
Memory cardinality: peakLiveValues=7  latency=5  work=12
  per-variable peak live: xs=4 y=3
  realized footprint (with eviction): 7
```

### Example 3: an affine system with heterogeneous index spaces (qr)

Modified Gram-Schmidt QR is a genuine SARE: its variables live in index spaces of
different ranks (`v`, `sr` are 3D; `r`, `sn`, `q` are 2D; `rho` is 1D), connected by
affine projection maps, with sqrt/division nonlinearities in the compute bodies.
No single global `tau` applies, so the spec is free-schedule-only:

```console
$ dfactl qr
spec: qr  (A = Q*R  (3x3 Modified Gram-Schmidt, SARE))
...
max|Q*R - A| = 1.77636e-15

Schedule legality: LEGAL  edges=72  minSlack(tau.theta)=1
Memory cardinality: peakLiveValues=11  latency=25  work=42
  per-variable peak live: q=3 r=2 rho=1 sn=2 sr=4 v=6
```

### Example 4: importing a .dfg graph

Any `.dfg` file (saved by the MLIR import tools or the graph API) can be lowered;
execution then follows only for graphs under the 200k-point execution cap, with
unsupported operators modeled as zero sources. The report is explicit about
coverage — unsupported operators are counted, not hidden:

```console
$ dfactl data/tosa/oneLayerMLP_tosa.dfg
imported .dfg: data/tosa/oneLayerMLP_tosa.dfg
nodes=9 edges=7
  lowered: CONSTANT=2 FUNCTION_ARGUMENT=1
  unsupported: CONV2D=1 FUNCTION_RETURN=1 RESHAPE=2
  index-space size (points): 9
  outputs: n7[1](func.return)
```

For a graph whose operators are fully supported (MATMUL, elementwise ops,
shape-preserving passthrough), the import produces a numerically runnable system:
`src/dfa/tests/sim/dfg_import_sim.cpp` builds an `arg[2x3] x const[3x2]` MATMUL
graph, round-trips it through `save()`/`load()`, and verifies `C[i][j] = 3.0`
end-to-end. Graphs above 200k index points are imported structurally but not
executed.

### Authoring a new spec

A `Spec` bundles a recurrence system, its canonical schedule, and an output
checker (see `include/dfa/sim/specs.hpp` for the three built-ins):

```cpp
Equation<double> eq{
    "c",                                                   // recurrence variable
    Domain(3).axis(0, 0, M).axis(1, 0, N).axis(2, 0, K),   // domain of computation
    { { "c", AffineDependency::shift({0, 0, -1}) },        // taps: what it reads
      { "a", AffineDependency::shift({0, -1, 0}) },
      { "b", AffineDependency::shift({-1, 0, 0}) } },
    [](const std::vector<double>& t, const IndexPoint&) {  // compute body
        return t[0] + t[1] * t[2]; },
    [C0](const IndexPoint& p) { return C0[p[0]][p[1]]; }   // boundary (k<0): C0
};
```

Triangular and otherwise non-rectangular domains add half-space constraints with
`Domain::add` (see the qr builder); affine (non-uniform) dependencies use
`AffineDependency::map(A, b)` for projections and permutations. To register a new
built-in, add a builder to `specs.hpp` and list it in `specNames()`/`buildSpec()`.
The sim tests (`src/dfa/tests/sim/`) reuse the builders, so a new spec gets CLI,
test, and documentation surfaces from one definition.

### Where the math comes from

The recurrence formulations are derived in `docs/SURE/`: `matmul.md` (including
the tensor/batched generalizations), `conv2d.md`, and `QR_decomposition.md`.
The scheduling theory (`tau.theta >= 1`, wavefronts, index-set splitting) is the
classic SURE framework of the domain flow architecture; see `ARCHITECTURE.md` and
`docs/domain-flow-history.md` for the broader context.
