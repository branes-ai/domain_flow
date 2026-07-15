// syr2_sure.cpp
//
// BLAS Level-2 symmetric rank-2 update as a System of Uniform Recurrence Equations
// (issue #36, docs/SURE/syr2.sure):
//
//   A(i,j) := A(i,j) + alpha*( x(i)y(j) + y(i)x(j) )   on the triangular domain j<=i.
//
// syr2 is syr with two vectors: two rank-1 outer products (x y^T and y x^T) fused
// into one symmetric update. Its structure is syr's -- a depth-2 feed/drain axis
// over the triangle -- but each cell needs four feeds (x(i), x(j), y(i), y(j)) from
// the two input vectors. It is the kernel behind syr2k and the LDL^T / eigensolver
// trailing updates.
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the parsed structure and the derived confluence face normals
//   - the numeric result A + alpha(x y^T + y x^T) over the lower triangle
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
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/syr2.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
        ok &= (spec.tau == std::vector<int>{ 1, 1, 1 });
        ok &= (spec.inputs.size() == 6);   // A, Alpha, X->xi, X->xj, Y->yi, Y->yj
        ok &= (spec.outputs.size() == 1);
        std::cout << "parsed structure (indices, tau, 6 inputs, 1 output): "
                  << (ok ? "PASS" : "FAIL") << "\n";

        if (spec.outputs.empty() || spec.inputs.empty())
            throw std::runtime_error("syr2: parse produced no input/output confluence");

        // ---- derived confluence orientation: outward face normals ----
        // A, alpha and all four vector feeds enter on the (i,j) face k=-1; the
        // updated lower triangle drains on the k=1 face.
        bool nok = true;
        for (const auto& in : spec.inputs) nok &= (in.normal == std::vector<int>{ 0, 0, -1 });
        nok &= (spec.outputs.front().normal == std::vector<int>{ 0, 0, 1 });
        std::cout << "derived face normals: " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: A(i,j) + alpha(x(i)y(j)+y(i)x(j)) on j<=i ----
        const size_t N = 3;
        const double A[3][3] = { { 1, 0, 0 }, { 2, 3, 0 }, { 4, 5, 6 } };
        const double X[3] = { 1, 2, 3 };
        const double Y[3] = { 2, 1, 4 };
        const double alpha = 1;
        double ref[3][3] = {};
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j <= i; ++j)
                ref[i][j] = A[i][j] + alpha * (X[i] * Y[j] + Y[i] * X[j]);

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        size_t checked = 0;
        bool numok = true;
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);   // Aout[i][j], j <= i
            double got = out.eval(sim, p);
            double want = ref[idx[0]][idx[1]];
            bool m = std::abs(got - want) < 1e-9;
            std::cout << "  Aout[" << idx[0] << "][" << idx[1] << "] = " << got
                      << "  (ref " << want << ")" << (m ? "" : "  MISMATCH") << "\n";
            numok &= m;
            ++checked;
        }
        numok &= (checked == N * (N + 1) / 2);   // lower triangle size
        std::cout << "parsed syr2 == A + alpha(x y^T + y x^T) (lower): " << (numok ? "PASS" : "FAIL") << "\n";
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
