// trsolve_sure.cpp
//
// Triangular solve -- forward substitution L x = b for a lower-triangular L, as a
// System of Affine Recurrence Equations (issue #49, docs/SURE/trsolve.sure):
//
//   x(i) = ( b(i) - sum_{j<i} L(i,j) x(j) ) / L(i,i),   0 <= i < N,
//
// the substitution engine behind every direct solver. trsv is trmv run backwards:
// the reduction reads the SOLVED x(j) via an affine tap onto the diagonal (j,j) --
// a SARE (the solved x is broadcast down the columns), and FREE-SCHEDULE-ONLY: the
// off-diagonal sum and the divide fuse at the same diagonal lattice point, a
// zero-slack self-dependence no linear tau can order.
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure (no linear tau) and the derived confluence face normals
//   - it reconstructs x and verifies L x == b on a lower-triangular L
//   - the free schedule is legal
//   - a flux-legal linear tau ([1,2,1]) is still rejected by the LEGALITY check
//     (the diagonal fuses reduction + divide) -- free-only for a reason distinct
//     from trmm's flux conflict
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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/trsolve.sure");

        // ---- parsed structure (free-only: no linear tau declared) ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= spec.tau.empty();            // no legal linear schedule exists
        ok &= (spec.inputs.size() == 3);   // L, Bin, reduction seed
        ok &= (spec.outputs.size() == 1);  // x on the diagonal
        std::cout << "parsed structure (indices, no tau, 3 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        if (spec.outputs.empty() || spec.inputs.empty())
            throw std::runtime_error("trsolve: parse produced no input/output confluence");

        // ---- derived confluence orientation: outward face normals ----
        // L and b enter on the (i,j) face k=-1; the reduction seed on j=-1; the
        // solution leaves on the DIAGONAL face i-j=0 (normal (-1,1,0), as in trmv).
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "L" || in.tensor == "Bin") nok &= (in.normal == std::vector<int>{ 0, 0, -1 });
            else                                        nok &= (in.normal == std::vector<int>{ 0, -1, 0 });  // seed
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ -1, 1, 0 });   // diagonal
        std::cout << "derived face normals (L/b on k=-1, diagonal solution): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- reconstruct x from the diagonal output, verify L x == b ----
        const size_t N = 4;
        const double L[4][4] = { { 2, 0, 0, 0 }, { 3, 4, 0, 0 }, { 5, 6, 7, 0 }, { 8, 9, 10, 11 } };
        const double b[4]    = { 2, 11, 38, 100 };
        double xv[4] = {};

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);   // Xout[i]
            xv[idx[0]] = out.eval(sim, p);
        }

        double maxErr = 0;
        for (size_t i = 0; i < N; ++i) {
            double lx = 0;
            for (size_t j = 0; j <= i; ++j) lx += L[i][j] * xv[j];   // (L x)(i)
            maxErr = std::max(maxErr, std::abs(lx - b[i]));
        }
        bool solved = maxErr < 1e-9;
        std::cout << "reconstructed L x == b: " << (solved ? "PASS" : "FAIL") << "  (max err " << maxErr << ")\n";
        std::cout << "  x = [" << xv[0] << "," << xv[1] << "," << xv[2] << "," << xv[3] << "]  (expect [1,2,3,4])\n";
        ok &= solved;

        // ---- the free schedule is legal ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        bool fl = checkLegality(spec.system, freeSched).legal;
        std::cout << "  free: " << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "free schedule legal: " << (fl ? "PASS" : "FAIL") << "\n";
        ok &= fl;

        // ---- free-only, distinct from trmm: the candidate linear tau = [1,2,1] is
        //      FLUX-legal (tau_j > tau_i for the diagonal outflux), yet the LEGALITY
        //      check rejects it -- the diagonal fuses the reduction and the divide at
        //      one lattice point (a zero-slack self-dependence). ----
        bool fluxLegal = true;
        try { validateSureFlux(spec, { 1, 2, 1 }); }   // must NOT throw: the faces are flux-legal
        catch (const std::exception&) { fluxLegal = false; }
        bool legalRejects = !checkLegality(spec.system, LinearSchedule({ 1, 2, 1 })).legal;
        std::cout << "  tau=[1,2,1]: " << checkLegality(spec.system, LinearSchedule({ 1, 2, 1 })) << "\n";
        std::cout << "free-only (flux-legal tau rejected by legality, not flux): "
                  << ((fluxLegal && legalRejects) ? "PASS" : "FAIL") << "\n";
        ok &= fluxLegal && legalRejects;

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
