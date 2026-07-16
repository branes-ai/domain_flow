// ldlt_sure.cpp
//
// LDL^T factorization A = L*D*L^T for a symmetric (possibly indefinite) matrix as a
// System of Affine Recurrence Equations (issue #44, docs/SURE/ldlt.sure):
//
//   a(i,j,k) = a(i,j,k-1) - a(i,k,k-1)*a(j,k,k-1)/a(k,k,k-1)   on the trailing lower
//   triangle j <= i, k <= j-1,
//
// the square-root-free sibling of Cholesky (same Schur update; factoring out the
// diagonal D avoids the sqrt, so it handles INDEFINITE A). Column j finalizes at
// k = j-1 giving D(j) = a(j,j,j-1) and L(i,j) = a(i,j,j-1)/a(j,j,j-1) for i > j. A SARE
// with a linear schedule tau = [1,1,2].
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure and the derived confluence face normals
//   - it reconstructs L (unit lower) and D and verifies L*D*L^T == A on an INDEFINITE A
//   - schedule legality (free and the canonical linear tau = [1,1,2])
//   - flux revalidation rejects a backward-flux tau
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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/ldlt.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.tau == std::vector<int>{ 1, 1, 2 });
        ok &= (spec.inputs.size() == 1);   // A
        ok &= (spec.outputs.size() == 2);  // L, D
        std::cout << "parsed structure (indices, tau, 1 input, 2 outputs): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        if (spec.outputs.empty() || spec.inputs.empty())
            throw std::runtime_error("ldlt: parse produced no input/output confluence");

        // ---- derived confluence orientation: outward face normals ----
        // A seeds the k=-1 halo; L on the super-diagonal j-k=1; D on the diagonal i-k=1.
        bool nok = (spec.inputs.front().normal == std::vector<int>{ 0, 0, -1 });
        for (const auto& out : spec.outputs) {
            if (out.tensor == "L")      nok &= (out.normal == std::vector<int>{ 0, -1, 1 });
            else if (out.tensor == "D") nok &= (out.normal == std::vector<int>{ -1, 0, 1 });
        }
        std::cout << "derived face normals (A seed, L/D super-diagonals): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- reconstruct L (unit lower) and D, verify L*D*L^T == A (indefinite A) ----
        const size_t N = 3;
        const double A[3][3] = { { 2, 2, 2 }, { 2, -1, -1 }, { 2, -1, 3 } };
        double Lm[3][3] = {}, Dv[3] = {};
        for (size_t i = 0; i < N; ++i) Lm[i][i] = 1.0;   // unit diagonal

        SureSimulator<double> sim(spec.system);
        for (const auto& out : spec.outputs) {
            for (const auto& p : out.region.enumerate()) {
                std::vector<long> idx = out.elemIndex(p);
                double v = out.eval(sim, p);
                if (out.tensor == "L") Lm[idx[0]][idx[1]] = v;
                else                   Dv[idx[0]] = v;     // D[i]
            }
        }

        double maxErr = 0;
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j) {
                double ldlt = 0;
                for (size_t k = 0; k < N; ++k) ldlt += Lm[i][k] * Dv[k] * Lm[j][k];   // (L D L^T)(i,j)
                maxErr = std::max(maxErr, std::abs(ldlt - A[i][j]));
            }
        bool factor = maxErr < 1e-9;
        // D must be indefinite here (mixed signs) -- the point of LDL^T over Cholesky
        bool indef = (Dv[0] > 0) && (Dv[1] < 0);
        std::cout << "reconstructed L*D*L^T == A: " << (factor ? "PASS" : "FAIL")
                  << "  (max err " << maxErr << ")\n";
        std::cout << "  L = [[1],[" << Lm[1][0] << ",1],[" << Lm[2][0] << "," << Lm[2][1] << ",1]]"
                  << "   D = diag(" << Dv[0] << "," << Dv[1] << "," << Dv[2] << ")"
                  << (indef ? "  [indefinite]" : "") << "\n";
        ok &= factor && indef;

        // ---- schedule legality: free and the canonical linear tau ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        LinearSchedule   good(spec.tau);          // [1,1,2]
        bool sok = true;
        sok &= checkLegality(spec.system, freeSched).legal;
        sok &= checkLegality(spec.system, good).legal;
        std::cout << "  free: "        << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "  tau=[1,1,2]: " << checkLegality(spec.system, good) << "\n";
        std::cout << "schedule analysis on parsed system: " << (sok ? "PASS" : "FAIL") << "\n";
        ok &= sok;

        // ---- a backward-flux override must be rejected by flux revalidation ----
        bool fok = false;
        try { validateSureFlux(spec, { 1, 1, 1 }); }
        catch (const std::exception& e) { fok = std::string(e.what()).find("flux") != std::string::npos; }
        std::cout << "flux revalidation rejects tau=[1,1,1]: " << (fok ? "PASS" : "FAIL") << "\n";
        ok &= fok;

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
