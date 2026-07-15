// trmv_sure.cpp
//
// BLAS Level-2 triangular matrix-vector product as a System of Uniform Recurrence
// Equations (issue #33, docs/SURE/trmv.sure):
//
//   x(i) := sum_{j <= i} T(i,j) * x(j),   0 <= i < N   (lower triangular)
//
// trmv is gemv on a TRIANGULAR domain (j <= i) — the first non-box index space in
// the catalog. T and the input vector both feed on the (i,j) face of a depth-1
// axis; the reduction sweeps +j over the variable extent j = 0..i, and the result
// leaves on the diagonal face i-j = 0.
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure and the derived confluence face normals (incl. the
//     diagonal output face)
//   - the numeric result x := T x against a direct lower-triangular reference
//   - schedule legality (free and the canonical linear tau = [1,2,1])
//   - flux revalidation rejects the box-default tau = [1,1,1] (zero diagonal flux)
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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/trmv.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.tau == std::vector<int>{ 1, 2, 1 });
        ok &= (spec.inputs.size() == 3);   // T, Xin, and the accumulator seed
        ok &= (spec.outputs.size() == 1);
        std::cout << "parsed structure (indices, tau, 3 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        // ---- derived confluence orientation: outward face normals ----
        // T and the input vector feed on the (i,j) face k=-1; the seed on j=-1;
        // the result leaves on the DIAGONAL face i-j=0 with normal (-1,1,0).
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "T" || in.tensor == "Xin") nok &= (in.normal == std::vector<int>{ 0, 0, -1 });
            else                                        nok &= (in.normal == std::vector<int>{ 0, -1, 0 }); // seed
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ -1, 1, 0 });   // diagonal
        std::cout << "derived face normals (incl. diagonal output): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: x(i) = sum_{j<=i} T(i,j) x(j) (mirrors the spec) ----
        const size_t N = 4;
        const double T[4][4] = {
            { 2, 0, 0, 0 }, { 3, 4, 0, 0 }, { 5, 6, 7, 0 }, { 8, 9, 10, 11 } };
        const double Xin[4] = { 1, 2, 3, 4 };
        double ref[4];
        for (size_t i = 0; i < N; ++i) {
            double s = 0;
            for (size_t j = 0; j <= i; ++j) s += T[i][j] * Xin[j];
            ref[i] = s;
        }

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        size_t checked = 0;
        bool numok = true;
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);   // Xout[i]
            double got = out.eval(sim, p);
            bool m = std::abs(got - ref[idx[0]]) < 1e-9;
            std::cout << "  Xout[" << idx[0] << "] = " << got << "  (ref " << ref[idx[0]] << ")"
                      << (m ? "" : "  MISMATCH") << "\n";
            numok &= m;
            ++checked;
        }
        numok &= (checked == N);   // one result per row, on the diagonal
        std::cout << "parsed trmv == T x reference: " << (numok ? "PASS" : "FAIL") << "\n";
        ok &= numok;

        // ---- schedule legality: free and the canonical linear tau ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        LinearSchedule   good(spec.tau);          // [1,2,1]
        bool sok = true;
        sok &= checkLegality(spec.system, freeSched).legal;
        sok &= checkLegality(spec.system, good).legal;
        std::cout << "  free: "        << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "  tau=[1,2,1]: " << checkLegality(spec.system, good) << "\n";
        std::cout << "schedule analysis on parsed system: " << (sok ? "PASS" : "FAIL") << "\n";
        ok &= sok;

        // ---- the box-default tau=[1,1,1] must be rejected: the diagonal output
        //      face needs tau_j > tau_i, so tau_j == tau_i gives zero outflux ----
        bool fok = false;
        try { validateSureFlux(spec, { 1, 1, 1 }); }
        catch (const std::exception& e) { fok = std::string(e.what()).find("flux") != std::string::npos; }
        std::cout << "flux revalidation rejects tau=[1,1,1] (diagonal): " << (fok ? "PASS" : "FAIL") << "\n";
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
