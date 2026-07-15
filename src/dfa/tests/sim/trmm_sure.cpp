// trmm_sure.cpp
//
// BLAS Level-3 triangular matrix-matrix product as a System of Uniform Recurrence
// Equations (issue #40, docs/SURE/trmm.sure):
//
//   B(i,j) := alpha * sum_{k <= i} T(i,k) B(k,j)      (T lower triangular),
//
// i.e. B := alpha*T*B in place. The reduction over k has a triangular extent
// (k = 0..i), a non-box 3-D domain. T(i,k) propagates +j; B(k,j) propagates +i
// (entering on the super-diagonal halo i-k=-1); the result leaves on the diagonal
// i-k=0.
//
// trmm is FREE-SCHEDULE-ONLY: the super-diagonal B-feed (i-k=-1) and the diagonal
// output (i-k=0) are parallel faces (same normal (-1,0,1)) with opposite flux, so no
// linear tau satisfies both -- the data-flow-earliest free schedule is required (as
// for the Modified-Gram-Schmidt qr.sure).
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure (no linear tau) and the derived confluence face normals
//   - the numeric result alpha*T*B (lower T) against a direct reference
//   - the free schedule is legal
//   - every candidate linear tau is rejected by flux revalidation (free-only)
//   - memory analysis agrees with the eviction run (under the free schedule)

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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/trmm.sure");

        // ---- parsed structure (free-only: no linear tau declared) ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= spec.tau.empty();            // no canonical linear schedule exists
        ok &= (spec.inputs.size() == 4);   // T, B, Alpha, seed
        ok &= (spec.outputs.size() == 1);
        std::cout << "parsed structure (indices, no tau, 4 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        if (spec.outputs.empty() || spec.inputs.empty())
            throw std::runtime_error("trmm: parse produced no input/output confluence");

        // ---- derived confluence orientation: outward face normals ----
        // T on j=-1; B on the super-diagonal i-k=-1; alpha/seed on k=-1; the result
        // leaves on the DIAGONAL face i-k=0 (same normal (-1,0,1) as the B feed).
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "T")          nok &= (in.normal == std::vector<int>{ 0, -1, 0 });
            else if (in.tensor == "B")     nok &= (in.normal == std::vector<int>{ -1, 0, 1 });
            else                           nok &= (in.normal == std::vector<int>{ 0, 0, -1 });   // Alpha, seed
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ -1, 0, 1 });   // diagonal
        std::cout << "derived face normals (super-diagonal B, diagonal output): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: alpha * sum_{k<=i} T(i,k) B(k,j) (mirrors the spec) ----
        const size_t M = 3, N = 2;
        const double T[3][3] = { { 2, 0, 0 }, { 3, 4, 0 }, { 5, 6, 7 } };
        const double B[3][2] = { { 1, 2 }, { 3, 4 }, { 5, 6 } };
        const double alpha = 1;
        double ref[3][2];
        for (size_t i = 0; i < M; ++i)
            for (size_t j = 0; j < N; ++j) {
                double s = 0;
                for (size_t k = 0; k <= i; ++k) s += T[i][k] * B[k][j];
                ref[i][j] = alpha * s;
            }

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        size_t checked = 0;
        bool numok = true;
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);   // Bout[i][j]
            double got = out.eval(sim, p);
            double want = ref[idx[0]][idx[1]];
            bool m = std::abs(got - want) < 1e-9;
            std::cout << "  Bout[" << idx[0] << "][" << idx[1] << "] = " << got
                      << "  (ref " << want << ")" << (m ? "" : "  MISMATCH") << "\n";
            numok &= m;
            ++checked;
        }
        numok &= (checked == M * N);
        std::cout << "parsed trmm == alpha*T*B reference: " << (numok ? "PASS" : "FAIL") << "\n";
        ok &= numok;

        // ---- the free schedule is legal ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        bool fl = checkLegality(spec.system, freeSched).legal;
        std::cout << "  free: " << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "free schedule legal: " << (fl ? "PASS" : "FAIL") << "\n";
        ok &= fl;

        // ---- free-only: every candidate linear tau is rejected by flux revalidation.
        //      The super-diagonal B feed needs tau_k < tau_i while the diagonal output
        //      needs tau_k > tau_i (parallel faces, same normal) -- no linear tau works.
        bool freeOnly = true;
        for (const std::vector<int>& tau : { std::vector<int>{ 2, 1, 1 },
                                             std::vector<int>{ 1, 1, 2 },
                                             std::vector<int>{ 1, 1, 1 } }) {
            bool rejected = false;
            try { validateSureFlux(spec, tau); }
            catch (const std::exception& e) { rejected = std::string(e.what()).find("flux") != std::string::npos; }
            freeOnly &= rejected;
        }
        std::cout << "free-only (all linear tau rejected by flux): " << (freeOnly ? "PASS" : "FAIL") << "\n";
        ok &= freeOnly;

        // ---- memory analysis agrees with the eviction run (free schedule) ----
        LivenessReport lr = sim.analyzeMemory(freeSched);
        long peak = 0;
        sim.run(freeSched, &peak);
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
