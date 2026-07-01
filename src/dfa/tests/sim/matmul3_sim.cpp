// matmul3_sim.cpp
//
// Functional SURE simulation of the three-input matmul  C = C0 + A*B.
//
// Uniform recurrence (the canonical systolic matmul, docs/SURE/matmul.md):
//   a(i,j,k) = a(i,j-1,k)                         boundary j<0 : A[i][k]
//   b(i,j,k) = b(i-1,j,k)                         boundary i<0 : B[k][j]
//   c(i,j,k) = c(i,j,k-1) + a(i,j-1,k)*b(i-1,j,k) boundary k<0 : C0[i][j]   <-- 3rd input
//   C[i][j]  = c(i,j,K-1)
//
// Verifies numeric output against a reference, then reports the peak live-value
// cardinality for a free (ASAP) schedule vs the linear schedule tau=[1,1,1].

#include <iostream>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_simulator.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

int main() {
    const int M = 2, N = 2, K = 2;
    std::vector<std::vector<double>> A  = {{1, 2}, {3, 4}};
    std::vector<std::vector<double>> B  = {{5, 6}, {7, 8}};
    std::vector<std::vector<double>> C0 = {{10, 20}, {30, 40}};

    RecurrenceSystem<double> sys;

    // a(i,j,k): propagate A along +j; seeded at j=0 from column of A.
    sys.add(Equation<double>{
        "a",
        Domain(3).axis(0, 0, M).axis(1, 0, N).axis(2, 0, K),
        { { "a", AffineDependency::shift({0, -1, 0}) } },
        [](const std::vector<double>& t, const IndexPoint&) { return t[0]; },
        [&](const IndexPoint& p) { return A[p[0]][p[2]]; }      // j<0 boundary: A[i][k]
    });

    // b(i,j,k): propagate B along +i; seeded at i=0 from row of B.
    sys.add(Equation<double>{
        "b",
        Domain(3).axis(0, 0, M).axis(1, 0, N).axis(2, 0, K),
        { { "b", AffineDependency::shift({-1, 0, 0}) } },
        [](const std::vector<double>& t, const IndexPoint&) { return t[0]; },
        [&](const IndexPoint& p) { return B[p[2]][p[1]]; }      // i<0 boundary: B[k][j]
    });

    // c(i,j,k): accumulate the product along +k; seeded at k=0 from C0.
    sys.add(Equation<double>{
        "c",
        Domain(3).axis(0, 0, M).axis(1, 0, N).axis(2, 0, K),
        { { "c", AffineDependency::shift({0,  0, -1}) },        // c(i,j,k-1)
          { "a", AffineDependency::shift({0, -1,  0}) },        // a(i,j-1,k)
          { "b", AffineDependency::shift({-1, 0,  0}) } },      // b(i-1,j,k)
        [](const std::vector<double>& t, const IndexPoint&) { return t[0] + t[1] * t[2]; },
        [&](const IndexPoint& p) { return C0[p[0]][p[1]]; }     // k<0 boundary: C0[i][j]
    });

    // ---- evaluate and check against reference C = C0 + A*B ----
    SureSimulator<double> simom(sys);
    bool ok = true;
    std::cout << "C = C0 + A*B:\n";
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            double ref = C0[i][j];
            for (int k = 0; k < K; ++k) ref += A[i][k] * B[k][j];
            double got = simom.eval("c", IndexPoint({ i, j, K - 1 }));
            std::cout << "  C[" << i << "][" << j << "] = " << got
                      << "  (ref " << ref << ")" << (std::abs(got - ref) < 1e-9 ? "" : "  MISMATCH");
            ok &= std::abs(got - ref) < 1e-9;
            std::cout << "\n";
        }
    }

    // ---- memory cardinality: free (ASAP) vs linear [1,1,1] ----
    SureSimulator<double> sim(sys);
    ExplicitSchedule freeSched = sim.computeFreeSchedule();
    LinearSchedule   linSched({1, 1, 1});

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

    // ---- schedule legality (tau.theta >= 1 on every dependency edge) ----
    LinearSchedule bad({1, 1, 0});   // tau.theta = 0 on the k-accumulation of c
    std::cout << "\nLegality:\n";
    std::cout << "  free          : " << checkLegality(sys, freeSched) << "\n";
    std::cout << "  linear [1,1,1]: " << checkLegality(sys, linSched)  << "\n";
    std::cout << "  linear [1,1,0]: " << checkLegality(sys, bad)       << "\n";
    ok &=  checkLegality(sys, freeSched).legal;
    ok &=  checkLegality(sys, linSched).legal;
    ok &= !checkLegality(sys, bad).legal;

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
