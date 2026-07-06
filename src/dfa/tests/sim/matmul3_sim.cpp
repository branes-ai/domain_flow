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
// The system itself is the canonical buildMatmul3() spec (specs.hpp); this test
// verifies its numeric output against the reference, then reports the peak
// live-value cardinality for a free (ASAP) schedule vs the linear schedule
// tau=[1,1,1], and checks schedule legality including a known-illegal tau.

#include <iostream>
#include <vector>
#include <cmath>
#include <dfa/sim/specs.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

int main() {
    Spec spec = buildMatmul3();
    const RecurrenceSystem<double>& sys = spec.system;

    // ---- evaluate and check against reference C = C0 + A*B ----
    SureSimulator<double> simom(sys);
    bool ok = spec.printOutputs(simom, std::cout);

    // ---- memory cardinality: free (ASAP) vs linear [1,1,1] ----
    SureSimulator<double> sim(sys);
    ExplicitSchedule freeSched = sim.computeFreeSchedule();
    LinearSchedule   linSched(spec.tau);   // canonical tau = [1,1,1]

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
