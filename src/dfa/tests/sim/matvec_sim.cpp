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
// streams without occupying memory.  The system itself is the canonical
// buildMatvec() spec (specs.hpp); this test verifies output, reports peak live
// values for the free (ASAP) schedule vs the linear schedule tau=[1,1] with the
// beta[y]=1 stage offset, and checks that the offset-less schedule is rejected.

#include <iostream>
#include <vector>
#include <cmath>
#include <dfa/sim/specs.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

int main() {
    Spec spec = buildMatvec();
    const RecurrenceSystem<double>& sys = spec.system;

    // ---- evaluate and check against reference y = A*x ----
    SureSimulator<double> simom(sys);
    bool ok = spec.printOutputs(simom, std::cout);

    // ---- memory cardinality: free (ASAP) vs linear [1,1] ----
    SureSimulator<double> sim(sys);
    ExplicitSchedule freeSched = sim.computeFreeSchedule();
    // y reads xs at the SAME index point (tau.theta = 0), so y needs a +1 stage
    // offset to make the linear schedule legal: beta[y] = 1 (the spec's canonical beta).
    LinearSchedule   linSched(spec.tau, spec.beta);

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
