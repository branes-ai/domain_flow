// lu_propagate_sure.cpp
//
// LU pivot PROPAGATION (issue #42, step 2, docs/SURE/lu_propagate.sure): one
// Gaussian-elimination step with the pivot broadcast uniformized into a
// nearest-neighbour propagation.
//
//   pr propagates the pivot row A(0,j) down +i; pv the pivot A(0,0) down +i; mc the
//   multiplier column A(i,0) across +j; r = A - (mc/pv)*pr is the one-step Schur
//   complement, applied once via the ger inject/drain pattern.
//
// Unlike step 1's SARE (affine pivot broadcast), every dependence here is a constant
// offset -- a pure SURE -- because the single step's pivot row/column come from the
// INPUT A and seed input-face propagations legally.
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure and the derived confluence face normals
//   - the numeric Schur complement R(i,j) = A(i,j) - (A(i,0)/A(0,0))*A(0,j)
//   - schedule legality (free and the canonical linear tau = [1,1,1]) -- a pure SURE
//   - the FREE SCHEDULE exhibits the propagation wavefront (a corner cell fires later
//     than the pivot cell, because the pivot/multiplier must hop across the array)
//   - memory analysis agrees with the eviction run

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/lu_propagate.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.tau == std::vector<int>{ 1, 1, 1 });
        ok &= (spec.inputs.size() == 5);   // A->r, Gate, A->pr, A->pv, A->mc
        ok &= (spec.outputs.size() == 1);
        std::cout << "parsed structure (indices, tau, 5 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        if (spec.outputs.empty() || spec.inputs.empty())
            throw std::runtime_error("lu_propagate: parse produced no input/output confluence");

        // ---- derived confluence orientation: outward face normals ----
        // A->r and the gate seed on k=-1; the pivot/pivot-row propagations enter on
        // i=-1; the multiplier-column propagation on j=-1; the result drains on k=1.
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "A" && in.normal == std::vector<int>{ 0, 0, -1 }) continue;   // r seed
            if (in.tensor == "Gate")                                          { nok &= (in.normal == std::vector<int>{ 0, 0, -1 }); continue; }
            if (in.tensor == "A" && in.normal == std::vector<int>{ -1, 0, 0 }) continue;   // pr, pv
            if (in.tensor == "A" && in.normal == std::vector<int>{ 0, -1, 0 }) continue;   // mc
            nok = false;
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ 0, 0, 1 });
        std::cout << "derived face normals (pr/pv down +i, mc across +j): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: one Schur step R(i,j) = A(i,j) - (A(i,0)/A(0,0))*A(0,j) ----
        const size_t N = 3;
        const double A[3][3] = { { 4, 3, 2 }, { 8, 7, 5 }, { 2, 3, 4 } };
        double ref[3][3];
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j)
                ref[i][j] = A[i][j] - (A[i][0] / A[0][0]) * A[0][j];

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        size_t checked = 0;
        bool numok = true;
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);   // R[i][j]
            double got = out.eval(sim, p);
            double want = ref[idx[0]][idx[1]];
            bool m = std::abs(got - want) < 1e-9;
            if (!m) std::cout << "  R[" << idx[0] << "][" << idx[1] << "] = " << got
                              << "  (ref " << want << ")  MISMATCH\n";
            numok &= m;
            ++checked;
        }
        numok &= (checked == N * N);
        std::cout << "parsed R == one Schur step (A - (A[:,0]/A[0,0]) A[0,:]): " << (numok ? "PASS" : "FAIL") << "\n";
        ok &= numok;

        // ---- schedule legality: pure SURE (free and the canonical linear tau) ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        LinearSchedule   good(spec.tau);          // [1,1,1]
        bool sok = true;
        sok &= checkLegality(spec.system, freeSched).legal;
        sok &= checkLegality(spec.system, good).legal;
        std::cout << "  free: "        << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "  tau=[1,1,1]: " << checkLegality(spec.system, good) << "\n";
        std::cout << "schedule analysis (a pure SURE): " << (sok ? "PASS" : "FAIL") << "\n";
        ok &= sok;

        // ---- the free schedule exhibits the propagation wavefront: the far corner
        //      cell fires strictly later than the pivot cell, because the pivot and
        //      multiplier must hop across the array (a broadcast would fire at once) ----
        long tCorner = freeSched.time("r", IndexPoint({ (int)N - 1, (int)N - 1, 0 }));
        long tPivot  = freeSched.time("r", IndexPoint({ 0, 0, 0 }));
        bool wave = tCorner > tPivot;
        std::cout << "propagation wavefront (r[N-1,N-1] fires later than r[0,0]): "
                  << (wave ? "PASS" : "FAIL") << "  (t_corner=" << tCorner << " > t_pivot=" << tPivot << ")\n";
        ok &= wave;

        // ---- memory analysis agrees with the eviction run ----
        LivenessReport lr = sim.analyzeMemory(good);
        long peak = 0;
        sim.run(good, &peak);
        bool mok = (peak == lr.peakLiveValues);
        std::cout << "memory analysis vs eviction run: " << (mok ? "PASS" : "FAIL")
                  << "  (peakLiveValues=" << lr.peakLiveValues << ")\n";
        ok &= mok;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
