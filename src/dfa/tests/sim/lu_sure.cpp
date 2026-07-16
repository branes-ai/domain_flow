// lu_sure.cpp
//
// LU factorization (Gaussian elimination, no pivoting) as a System of Affine
// Recurrence Equations (issue #42, step 1, docs/SURE/lu.sure):
//
//   A = L * U,  L unit-lower-triangular, U upper-triangular,
//
// via the right-looking Schur elimination a(i,j,k) = a(i,j,k-1) -
// (a(i,k,k-1)/a(k,k,k-1)) * a(k,j,k-1) on the strict trailing domain k <= i-1,
// k <= j-1. It is a SARE (affine pivot / row / column taps) but, because every
// consumer sits strictly below-and-right of the pivot, it admits a LINEAR schedule
// tau = [1,1,2] as well as the free schedule.
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure and the derived confluence face normals
//   - it reconstructs L and U from the two output faces and verifies L*U == A
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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/lu.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.tau == std::vector<int>{ 1, 1, 2 });
        ok &= (spec.inputs.size() == 1);   // A
        ok &= (spec.outputs.size() == 2);  // U, L
        std::cout << "parsed structure (indices, tau, 1 input, 2 outputs): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        if (spec.outputs.empty() || spec.inputs.empty())
            throw std::runtime_error("lu: parse produced no input/output confluence");

        // ---- derived confluence orientation: outward face normals ----
        // A seeds the k=-1 halo; U leaves on the super-diagonal i-k=1; L on j-k=1.
        bool nok = true;
        nok &= (spec.inputs.front().normal == std::vector<int>{ 0, 0, -1 });
        for (const auto& out : spec.outputs) {
            if (out.tensor == "U")      nok &= (out.normal == std::vector<int>{ -1, 0, 1 });
            else if (out.tensor == "L") nok &= (out.normal == std::vector<int>{ 0, -1, 1 });
        }
        std::cout << "derived face normals (A seed, U/L super-diagonals): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- reconstruct L (unit lower) and U (upper) from the outputs, verify L*U == A ----
        const size_t N = 3;
        const double A[3][3] = { { 4, 3, 2 }, { 8, 7, 5 }, { 2, 3, 4 } };
        double Lm[3][3] = {}, Um[3][3] = {};
        for (size_t i = 0; i < N; ++i) Lm[i][i] = 1.0;   // unit diagonal

        SureSimulator<double> sim(spec.system);
        for (const auto& out : spec.outputs) {
            for (const auto& p : out.region.enumerate()) {
                std::vector<long> idx = out.elemIndex(p);   // [i][j]
                double v = out.eval(sim, p);
                if (out.tensor == "U") Um[idx[0]][idx[1]] = v;
                else                   Lm[idx[0]][idx[1]] = v;
            }
        }

        double maxErr = 0;
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j) {
                double lu = 0;
                for (size_t k = 0; k < N; ++k) lu += Lm[i][k] * Um[k][j];
                maxErr = std::max(maxErr, std::abs(lu - A[i][j]));
            }
        bool factor = maxErr < 1e-9;
        std::cout << "reconstructed L*U == A: " << (factor ? "PASS" : "FAIL")
                  << "  (max err " << maxErr << ")\n";
        std::cout << "  U = [[" << Um[0][0] << "," << Um[0][1] << "," << Um[0][2] << "],["
                  << Um[1][1] << "," << Um[1][2] << "],[" << Um[2][2] << "]]\n";
        std::cout << "  L = [[" << Lm[1][0] << "],[" << Lm[2][0] << "," << Lm[2][1] << "]]\n";
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
