// scal_sure.cpp
//
// BLAS Level-1 scale as a System of Uniform Recurrence Equations (issue #26,
// docs/SURE/scal.md).  scal  r := alpha*x  is a fully-parallel elementwise scale
// -- exactly axpy specialized to a zero addend:
//
//   a(i,j) = a(i-1,j);                         // scalar alpha pipelined across lanes
//   x(i,j) = 0;                                // one-shot injection (additive identity)
//   r(i,j) = r(i,j-1) + a(i-1,j) * x(i,j-1);   // result stream, seeded 0 (no addend)
//
// alpha is projected onto the i = -1 edge and pipelined across the lanes; x is
// injected on the j = -1 halo and consumed once; the length-N result rides the
// i axis and leaves through the terminal j = 1 face.  The result stream r is
// seeded with the additive identity 0 (unlike axpy, whose stream is seeded with
// the incoming vector Y).
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the derived confluence structure (input/output face normals)
//   - the numeric result R = alpha*X against a direct reference
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

// docs/SURE/scal.sure is loaded from the source tree (SURE_DOCS_DIR is set by
// CMake) so the executable doc stays the single source of truth.
int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/scal.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j" });
        ok &= (spec.tau == std::vector<int>{ 1, 1 });
        ok &= (spec.inputs.size() == 3);   // Alpha, X, and the result seed
        ok &= (spec.outputs.size() == 1);

        // ---- derived confluence orientation: outward face normals ----
        // alpha is projected onto the i = -1 edge; x and the result seed enter on
        // the -j halo; the scaled vector exits on the +j terminal face.
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "Alpha")  nok &= (in.normal == std::vector<int>{ -1, 0 });  // project
            else if (in.tensor == "X") nok &= (in.normal == std::vector<int>{ 0, -1 });  // inject
            else                       nok &= (in.normal == std::vector<int>{ 0, -1 });  // seed (const)
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ 0, 1 });                // result
        std::cout << "derived face normals (Alpha,X,seed,R): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: R = alpha*X (alpha, X mirror the spec) ----
        const int    alpha = 3;
        const double X[4] = { 1, 2, 3, 4 };

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        int checked = 0;
        for (const auto& p : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(p);
            double got = out.eval(sim, p);
            double ref = alpha * X[idx[0]];
            bool m = std::abs(got - ref) < 1e-9;
            std::cout << "  R[" << idx[0] << "] = " << got << "  (ref " << ref << ")"
                      << (m ? "" : "  MISMATCH") << "\n";
            ok &= m;
            ++checked;
        }
        ok &= (checked == 4);
        std::cout << "parsed scal == alpha*X reference: " << (ok ? "PASS" : "FAIL") << "\n";

        // ---- schedule legality: free and the canonical linear tau ----
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        LinearSchedule   good(spec.tau);          // [1,1]
        bool sok = true;
        sok &= checkLegality(spec.system, freeSched).legal;
        sok &= checkLegality(spec.system, good).legal;
        std::cout << "  free: "      << checkLegality(spec.system, freeSched) << "\n";
        std::cout << "  tau=[1,1]: " << checkLegality(spec.system, good) << "\n";
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
