// cholesky_sure.cpp
//
// Cholesky factorization A = L*L^T for a symmetric positive-definite A as a System of
// Affine Recurrence Equations (issue #43, docs/SURE/cholesky.sure):
//
//   a(i,j,k) = a(i,j,k-1) - a(i,k,k-1)*a(j,k,k-1)/a(k,k,k-1)   on the trailing lower
//   triangle j <= i, k <= j-1,
//
// the right-looking symmetric elimination -- LU specialized to A = A^T (one factor,
// no pivoting). Like lu.sure it is a SARE (affine column/pivot taps) but admits a
// LINEAR schedule tau = [1,1,2] because every consumer sits below-and-right of the
// pivot. A single output face gives all of L: L(i,j) = a(i,j,j-1)/sqrt(a(j,j,j-1)),
// which yields sqrt(pivot) on the diagonal and the multiplier below.
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure and the derived confluence face normals
//   - it reconstructs L and verifies L*L^T == A
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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/cholesky.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.tau == std::vector<int>{ 1, 1, 2 });
        ok &= (spec.inputs.size() == 1);   // A
        ok &= (spec.outputs.size() == 1);  // L
        std::cout << "parsed structure (indices, tau, 1 input, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        if (spec.outputs.empty() || spec.inputs.empty())
            throw std::runtime_error("cholesky: parse produced no input/output confluence");

        // ---- derived confluence orientation: outward face normals ----
        // A seeds the k=-1 halo; L leaves on the super-diagonal j-k=1.
        bool nok = (spec.inputs.front().normal == std::vector<int>{ 0, 0, -1 })
                && (spec.outputs.front().normal == std::vector<int>{ 0, -1, 1 });
        std::cout << "derived face normals (A seed, L super-diagonal): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- reconstruct L (lower) from the output, verify L*L^T == A ----
        const size_t N = 3;
        const double A[3][3] = { { 4, 2, 2 }, { 2, 5, 3 }, { 2, 3, 6 } };
        double Lm[3][3] = {};

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);   // [i][j], j <= i
            Lm[idx[0]][idx[1]] = out.eval(sim, p);
        }

        double maxErr = 0;
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j) {
                double llt = 0;
                for (size_t k = 0; k < N; ++k) llt += Lm[i][k] * Lm[j][k];   // (L L^T)(i,j)
                maxErr = std::max(maxErr, std::abs(llt - A[i][j]));
            }
        bool factor = maxErr < 1e-9;
        std::cout << "reconstructed L*L^T == A: " << (factor ? "PASS" : "FAIL")
                  << "  (max err " << maxErr << ")\n";
        std::cout << "  L = [[" << Lm[0][0] << "],[" << Lm[1][0] << "," << Lm[1][1] << "],["
                  << Lm[2][0] << "," << Lm[2][1] << "," << Lm[2][2] << "]]\n";
        ok &= factor;

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
