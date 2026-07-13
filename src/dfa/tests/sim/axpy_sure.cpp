// axpy_sure.cpp
//
// BLAS Level-1 axpy as a System of Uniform Recurrence Equations (issue #22,
// docs/SURE/axpy.md).  axpy  y := alpha*x + y  is a fully-parallel elementwise
// map; the executable spec docs/SURE/axpy.sure rides the length-N vector on the
// i axis and turns the map into a two-cell systolic pipeline on j so the result
// leaves through an oriented output face:
//
//   a(i,j) = a(i-1,j);                         // scalar alpha pipelined across lanes
//   x(i,j) = 0;                                // one-shot injection (additive identity in-domain)
//   y(i,j) = y(i,j-1) + a(i-1,j) * x(i,j-1);   // y streams +j, picks up alpha*x once
//
// All three operands are uniform flows: alpha is PROJECTED onto the i = -1 edge
// and pipelined across the lanes (no broadcast constant in any equation body);
// x enters on the j = -1 halo (reads X[i]); y is seeded there with Y[i] and
// drains to the terminal j = 1 face as R[i].
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the derived confluence structure (input/output face normals)
//   - the numeric result R = alpha*X + Y against a direct reference
//   - schedule legality (free and the canonical linear tau = [1,1])
//   - memory analysis agrees with the eviction run

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

// docs/SURE/axpy.sure is loaded from the source tree (SURE_DOCS_DIR is set by
// CMake) so the executable doc stays the single source of truth.
int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/axpy.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j" });
        ok &= (spec.tau == std::vector<int>{ 1, 1 });
        ok &= (spec.inputs.size() == 3);   // Alpha, X, Y -- all three operands enter through confluences
        ok &= (spec.outputs.size() == 1);

        // ---- derived confluence orientation: outward face normals ----
        // alpha is projected onto the i = -1 edge and pipelined across lanes, so
        // its influx face points along -i; x and y enter on the -j halo.
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "Alpha")  nok &= (in.normal == std::vector<int>{ -1, 0 });  // project on i = -1
            else if (in.tensor == "X") nok &= (in.normal == std::vector<int>{ 0, -1 });  // inject on j = -1
            else if (in.tensor == "Y") nok &= (in.normal == std::vector<int>{ 0, -1 });  // seed on j = -1
            else                       nok = false;
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ 0, 1 });                // drain at j = 1
        std::cout << "derived face normals (Alpha,X,Y,R): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: R = alpha*X + Y (alpha, X, Y mirror the spec) ----
        const int    alpha = 2;
        const double X[4] = {  1,  2,  3,  4 };
        const double Y[4] = { 10, 20, 30, 40 };

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        int checked = 0;
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);
            double got = out.eval(sim, p);
            double ref = alpha * X[idx[0]] + Y[idx[0]];
            bool m = std::abs(got - ref) < 1e-9;
            std::cout << "  R[" << idx[0] << "] = " << got << "  (ref " << ref << ")"
                      << (m ? "" : "  MISMATCH") << "\n";
            ok &= m;
            ++checked;
        }
        ok &= (checked == 4);
        std::cout << "parsed axpy == alpha*X + Y reference: " << (ok ? "PASS" : "FAIL") << "\n";

        // ---- schedule legality: free and the canonical linear tau ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        LinearSchedule   good(spec.tau);          // [1,1]
        bool sok = true;
        sok &= checkLegality(spec.system, freeSched).legal;
        sok &= checkLegality(spec.system, good).legal;
        std::cout << "  free: "        << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "  tau=[1,1]: "   << checkLegality(spec.system, good) << "\n";
        std::cout << "schedule analysis on parsed system: " << (sok ? "PASS" : "FAIL") << "\n";
        ok &= sok;

        // ---- a backward-flux override must be rejected by flux revalidation ----
        bool fok = false;
        try { validateSureFlux(spec, { 1, -1 }); }
        catch (const std::exception& e) { fok = std::string(e.what()).find("flux") != std::string::npos; }
        std::cout << "flux revalidation for overridden tau=[1,-1]: " << (fok ? "PASS" : "FAIL") << "\n";
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
