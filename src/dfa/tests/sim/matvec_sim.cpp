// matvec_sim.cpp
//
// Functional SURE simulation of the matrix-vector product  y = A*x   (A is MxN).
//
// Uniform recurrence over the index space (i,k), i = row, k = reduction:
//   xs(i,k) = xs(i-1,k)                  boundary i<0 : x[k]      (broadcast x down the rows)
//   y(i,k)  = y(i,k-1) + A[i][k]*xs(i,k) boundary k<0 : 0         (accumulate the dot product)
//   y[i]    = y(i,N-1)
//
// A is supplied as a rank-0 (input) equation: every access is a boundary, so it
// streams without occupying memory.  Verifies output, then reports peak live
// values for the free (ASAP) schedule vs the linear schedule tau=[1,1].

#include <iostream>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_simulator.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

int main() {
    const int M = 2, N = 3;
    std::vector<std::vector<double>> A = {{1, 2, 3}, {4, 5, 6}};
    std::vector<double> x = {1, 0, -1};

    RecurrenceSystem<double> sys;

    // A: pure input operand (empty domain => every read is a boundary).
    sys.add(Equation<double>{
        "A", Domain{}, {},
        nullptr,
        [&](const IndexPoint& p) { return A[p[0]][p[1]]; }     // (i,k) -> A[i][k]
    });

    // xs(i,k): broadcast x[k] down the rows i.
    sys.add(Equation<double>{
        "xs",
        Domain(2).axis(0, 0, M).axis(1, 0, N),
        { { "xs", AffineDependency::shift({-1, 0}) } },
        [](const std::vector<double>& t, const IndexPoint&) { return t[0]; },
        [&](const IndexPoint& p) { return x[p[1]]; }           // i<0 boundary: x[k]
    });

    // y(i,k): accumulate A[i][k]*xs(i,k) along +k.
    sys.add(Equation<double>{
        "y",
        Domain(2).axis(0, 0, M).axis(1, 0, N),
        { { "y",  AffineDependency::shift({0, -1}) },           // y(i,k-1)
          { "A",  AffineDependency::shift({0,  0}) },           // A(i,k)   (identity -> input)
          { "xs", AffineDependency::shift({0,  0}) } },         // xs(i,k)
        [](const std::vector<double>& t, const IndexPoint&) { return t[0] + t[1] * t[2]; },
        [](const IndexPoint&) { return 0.0; }                  // k<0 boundary: accumulator init 0
    });

    // ---- evaluate and check against reference y = A*x ----
    SureSimulator<double> simom(sys);
    bool ok = true;
    std::cout << "y = A*x:\n";
    for (int i = 0; i < M; ++i) {
        double ref = 0;
        for (int k = 0; k < N; ++k) ref += A[i][k] * x[k];
        double got = simom.eval("y", IndexPoint({ i, N - 1 }));
        std::cout << "  y[" << i << "] = " << got << "  (ref " << ref << ")"
                  << (std::abs(got - ref) < 1e-9 ? "" : "  MISMATCH") << "\n";
        ok &= std::abs(got - ref) < 1e-9;
    }

    // ---- memory cardinality: free (ASAP) vs linear [1,1] ----
    SureSimulator<double> sim(sys);
    ExplicitSchedule freeSched = sim.computeFreeSchedule();
    // y reads xs at the SAME index point (tau.theta = 0), so y needs a +1 stage
    // offset to make the linear schedule legal: beta[y] = 1.
    LinearSchedule   linSched({1, 1}, { { "y", 1 } });

    LivenessReport fr = sim.analyzeMemory(freeSched);
    LivenessReport lr = sim.analyzeMemory(linSched);

    long freePeak = 0, linPeak = 0;
    sim.run(freeSched, &freePeak);
    sim.run(linSched,  &linPeak);

    auto report = [](const char* tag, const LivenessReport& r, long observed) {
        std::cout << "  " << tag
                  << ": peakLiveValues=" << r.peakLiveValues
                  << "  latency=" << r.latency
                  << "  work=" << r.totalValues
                  << "  (run footprint=" << observed << ")\n";
    };
    std::cout << "\nMemory cardinality:\n";
    report("free  ", fr, freePeak);
    report("linear", lr, linPeak);

    ok &= (freePeak == fr.peakLiveValues);
    ok &= (linPeak  == lr.peakLiveValues);

    // ---- schedule legality: y reads xs at the same point, so a bare tau=[1,1]
    //      has tau.theta = 0 on that edge and must be rejected; beta[y]=1 fixes it.
    LinearSchedule bad({1, 1});
    std::cout << "\nLegality:\n";
    std::cout << "  free                : " << checkLegality(sys, freeSched) << "\n";
    std::cout << "  linear [1,1] beta[y]=1: " << checkLegality(sys, linSched) << "\n";
    std::cout << "  linear [1,1] no beta: " << checkLegality(sys, bad) << "\n";
    ok &=  checkLegality(sys, freeSched).legal;
    ok &=  checkLegality(sys, linSched).legal;
    ok &= !checkLegality(sys, bad).legal;

    // run() must refuse the illegal schedule up front with the full report.
    bool threw = false;
    try {
        sim.run(bad);
    } catch (const std::exception& e) {
        threw = true;
        std::cout << "  run([1,1]) rejected: " << e.what() << "\n";
    }
    ok &= threw;

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
