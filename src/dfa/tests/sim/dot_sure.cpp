// dot_sure.cpp
//
// BLAS Level-1 dot product as a System of Uniform Recurrence Equations (issue
// #23, docs/SURE/dot.md).  dot  s = sum_i X[i]*Y[i]  is the canonical reduction:
// a linear chain of multiply-adds accumulating along +i,
//
//   s(i,j) = s(i-1,j) + x(i,j-1) * y(i,j-1);   // seed s(-1,.) = 0
//
// with the scalar result leaving through the single terminal point i = N-1.  The
// operands are indexed by the reduction coordinate i, so they enter on a depth-1
// FEED axis j: X[i], Y[i] are injected on the j = -1 halo and consumed at their
// MAC cell (a reduction reuses no operand, so the feed is one cell deep).
//
// This test parses the executable doc (the single source of truth) and checks:
//   - the derived confluence structure (input/output face normals)
//   - the numeric result s = X . Y against a direct reference
//   - schedule legality (free and the canonical linear tau = [1,1])
//   - the reduction footprint: peak live values is O(1) (an accumulator)

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

// docs/SURE/dot.sure is loaded from the source tree (SURE_DOCS_DIR is set by
// CMake) so the executable doc stays the single source of truth.
int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/dot.sure");

        // ---- parsed structure ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j" });
        ok &= (spec.tau == std::vector<int>{ 1, 1 });
        ok &= (spec.inputs.size() == 3);   // X, Y, and the accumulator seed
        ok &= (spec.outputs.size() == 1);

        // ---- derived confluence orientation: outward face normals ----
        // X and Y are fed on the -j halo; the accumulator seed enters on the -i
        // face; the scalar result exits on the +i terminal face.
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "X")      nok &= (in.normal == std::vector<int>{ 0, -1 });   // feed
            else if (in.tensor == "Y") nok &= (in.normal == std::vector<int>{ 0, -1 });   // feed
            else                       nok &= (in.normal == std::vector<int>{ -1, 0 });   // seed (const)
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ 1, 0 });                 // result
        std::cout << "derived face normals (X,Y,seed,S): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: s = sum_i X[i]*Y[i] (X, Y mirror the spec) ----
        const double X[4] = { 1, 2, 3, 4 };
        const double Y[4] = { 4, 3, 2, 1 };
        double ref = 0;
        for (int i = 0; i < 4; ++i) ref += X[i] * Y[i];

        SureSimulator<double> sim(spec.system);
        const SureOutput& out = spec.outputs.front();
        int checked = 0;
        for (const auto& p : out.region.enumerate()) {
            double got = out.eval(sim, p);
            bool m = std::abs(got - ref) < 1e-9;
            std::cout << "  S[0] = " << got << "  (ref " << ref << ")" << (m ? "" : "  MISMATCH") << "\n";
            ok &= m;
            ++checked;
        }
        ok &= (checked == 1);   // a scalar leaves through a single terminal point
        std::cout << "parsed dot == X . Y reference: " << (ok ? "PASS" : "FAIL") << "\n";

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
        try { validateSureFlux(spec, { -1, 1 }); }
        catch (const std::exception& e) { fok = std::string(e.what()).find("flux") != std::string::npos; }
        std::cout << "flux revalidation for overridden tau=[-1,1]: " << (fok ? "PASS" : "FAIL") << "\n";
        ok &= fok;

        // ---- reduction footprint: an O(1) accumulator, analysis == run ----
        LivenessReport lr = sim.analyzeMemory(good);
        long peak = 0;
        sim.run(good, &peak);
        bool mok = (peak == lr.peakLiveValues) && (lr.peakLiveValues <= 2);
        std::cout << "memory analysis vs eviction run: " << (mok ? "PASS" : "FAIL")
                  << "  (peakLiveValues=" << lr.peakLiveValues << ", accumulator footprint)\n";
        ok &= mok;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
