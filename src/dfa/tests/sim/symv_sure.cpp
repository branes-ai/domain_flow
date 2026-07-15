// symv_sure.cpp
//
// BLAS Level-2 symmetric matrix-vector product as a System of Uniform Recurrence
// Equations (issue #34, docs/SURE/symv.sure):
//
//   y(i) := beta*y(i) + alpha*sum_j A(i,j) x(j),   A = A^T, one stored triangle.
//
// symv is gemv for a symmetric A stored as a single (lower) triangle. The symmetry
// lives in the confluence: a two-face feed reads each stored element A(i,j) twice —
// the lower cells read A[i][j], the upper cells read the reflected A[j][i] — so cell
// (i,j) always sees A_sym(i,j) and the reduction is the ordinary gemv sweep. This
// realizes "route each stored A(i,j) to two accumulations" (y(i) and y(j)).
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure and the derived confluence face normals (two A feeds)
//   - the numeric result against a dense symmetric reference (built from the lower
//     triangle by reflection)
//   - schedule legality (free and the canonical linear tau = [1,1,1])
//   - flux revalidation rejects a backward-flux tau
//   - memory analysis agrees with the eviction run

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/symv.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.tau == std::vector<int>{ 1, 1, 1 });
        ok &= (spec.inputs.size() == 7);   // two A feeds, X, Alpha, Beta, Yin, seed
        ok &= (spec.outputs.size() == 1);
        std::cout << "parsed structure (indices, tau, 7 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        if (spec.outputs.empty() || spec.inputs.empty())
            throw std::runtime_error("symv: parse produced no input/output confluence");

        // ---- derived confluence orientation: outward face normals ----
        // Both A feeds and alpha enter on the (i,j) face k=-1; x and beta on the i=-1
        // face; y_in and the seed on j=-1; the result exits on the terminal j=N-1.
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "A" || in.tensor == "Alpha")     nok &= (in.normal == std::vector<int>{ 0, 0, -1 });
            else if (in.tensor == "X" || in.tensor == "Beta") nok &= (in.normal == std::vector<int>{ -1, 0, 0 });
            else                                              nok &= (in.normal == std::vector<int>{ 0, -1, 0 }); // Yin, seed
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ 0, 1, 0 });
        std::cout << "derived face normals (two symmetric A feeds): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: y = beta*y + alpha*A_sym*x (A_sym reflected from lower) ----
        const size_t N = 3;
        const double Alo[3][3] = { { 2, 0, 0 }, { 3, 4, 0 }, { 5, 6, 7 } };  // stored lower triangle
        double Asym[3][3];
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j)
                Asym[i][j] = (j <= i) ? Alo[i][j] : Alo[j][i];   // reflect
        const double X[3] = { 1, 2, 3 };
        const double alpha = 2, beta = 10;
        const double Yin[3] = { 1, 2, 3 };
        double ref[3];
        for (size_t i = 0; i < N; ++i) {
            double s = 0;
            for (size_t j = 0; j < N; ++j) s += Asym[i][j] * X[j];
            ref[i] = beta * Yin[i] + alpha * s;
        }

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        size_t checked = 0;
        bool numok = true;
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);   // Y[i]
            double got = out.eval(sim, p);
            bool m = std::abs(got - ref[idx[0]]) < 1e-9;
            std::cout << "  Y[" << idx[0] << "] = " << got << "  (ref " << ref[idx[0]] << ")"
                      << (m ? "" : "  MISMATCH") << "\n";
            numok &= m;
            ++checked;
        }
        numok &= (checked == N);
        std::cout << "parsed symv == beta*y + alpha*A_sym*x reference: " << (numok ? "PASS" : "FAIL") << "\n";
        ok &= numok;

        // ---- schedule legality: free and the canonical linear tau ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        LinearSchedule   good(spec.tau);          // [1,1,1]
        bool sok = true;
        sok &= checkLegality(spec.system, freeSched).legal;
        sok &= checkLegality(spec.system, good).legal;
        std::cout << "  free: "        << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "  tau=[1,1,1]: " << checkLegality(spec.system, good) << "\n";
        std::cout << "schedule analysis on parsed system: " << (sok ? "PASS" : "FAIL") << "\n";
        ok &= sok;

        // ---- a backward-flux override must be rejected by flux revalidation ----
        bool fok = false;
        try { validateSureFlux(spec, { -1, -1, -1 }); }
        catch (const std::exception& e) { fok = std::string(e.what()).find("flux") != std::string::npos; }
        std::cout << "flux revalidation for overridden tau=[-1,-1,-1]: " << (fok ? "PASS" : "FAIL") << "\n";
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
