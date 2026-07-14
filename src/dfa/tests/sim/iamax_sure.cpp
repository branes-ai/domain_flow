// iamax_sure.cpp
//
// BLAS Level-1 iamax (index of the largest-magnitude element) as a System of
// Uniform Recurrence Equations (issue #30, docs/SURE/iamax.md).  iamax is a
// SELECT reduction: it carries an (index, value) pair and keeps the pair with the
// larger magnitude, using the exact selection built-ins gt(a,b) and
// select(c,x,y) added to the DSL for this class of operator:
//
//   mval(i,j) = select(gt(av(i,j-1), mval(i-1,j)), av(i,j-1), mval(i-1,j));
//   midx(i,j) = select(gt(av(i,j-1), mval(i-1,j)), idx(i,j-1), midx(i-1,j));
//
// The magnitude enters via the abs input-face prologue; the index enters as data
// (Idx[i] = i projected onto the feed halo).  Strict gt keeps the earlier index
// on ties (matching BLAS).
//
// This test parses the executable doc and additionally checks tie-breaking and a
// last-element winner on inline specs, plus schedule legality and memory.

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

// evaluate the single scalar argmax index a parsed iamax-style spec produces
static double runArgmax(const SureSpec& spec) {
    SureSimulator<double> sim(spec.system);
    const SureOutput& out = spec.outputs.front();
    double v = 0;
    for (const auto& p : out.region.enumerate()) v = out.eval(sim, p);   // single terminal point
    return v;
}

int main() {
    bool ok = true;
    try {
        SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/iamax.sure");

        // ---- parsed structure: X, Idx, two seeds; one output ----
        ok &= (spec.indexNames == std::vector<std::string>{ "i", "j" });
        ok &= (spec.tau == std::vector<int>{ 1, 1 });
        ok &= (spec.inputs.size() == 4);   // X, Idx, mval-seed, midx-seed
        ok &= (spec.outputs.size() == 1);

        // ---- derived confluence orientation: outward face normals ----
        bool nok = true;
        for (const auto& in : spec.inputs) {
            if (in.tensor == "X" || in.tensor == "Idx")
                nok &= (in.normal == std::vector<int>{ 0, -1 });   // feed
            else
                nok &= (in.normal == std::vector<int>{ -1, 0 });   // seed pair
        }
        nok &= (spec.outputs.front().normal == std::vector<int>{ 1, 0 });   // result
        std::cout << "derived face normals (X,Idx feed; seeds; I): " << (nok ? "PASS" : "FAIL") << "\n";
        ok &= nok;

        // ---- numeric result: argmax|X| for X = {1,-5,3,-2} is index 1 ----
        double got = runArgmax(spec);
        bool m = std::abs(got - 1.0) < 1e-9;
        std::cout << "  iamax = " << got << "  (ref 1)" << (m ? "" : "  MISMATCH") << "\n";
        ok &= m;
        std::cout << "parsed iamax == argmax|X|: " << (ok ? "PASS" : "FAIL") << "\n";

        // ---- schedule legality + memory (on the parsed system) ----
        SureSimulator<double> sim(spec.system);
        ExplicitSchedule freeSched = sim.computeFreeSchedule();
        LinearSchedule   good(spec.tau);
        bool sok = checkLegality(spec.system, freeSched).legal && checkLegality(spec.system, good).legal;
        std::cout << "  tau=[1,1]: " << checkLegality(spec.system, good) << "\n";
        LivenessReport lr = sim.analyzeMemory(good);
        long peak = 0; sim.run(good, &peak);
        sok &= (peak == lr.peakLiveValues);
        std::cout << "schedule legality + memory: " << (sok ? "PASS" : "FAIL")
                  << "  (peakLiveValues=" << lr.peakLiveValues << ")\n";
        ok &= sok;

        // ---- a backward-flux override must be rejected ----
        bool fok = false;
        try { validateSureFlux(spec, { -1, 1 }); }
        catch (const std::exception& e) { fok = std::string(e.what()).find("flux") != std::string::npos; }
        std::cout << "flux revalidation for overridden tau=[-1,1]: " << (fok ? "PASS" : "FAIL") << "\n";
        ok &= fok;

        // ---- gt/select semantics: tie-breaking and a last-element winner ----
        // Same recurrence as the doc spec, varying only the data, to pin down the
        // exact selection behaviour independent of the file's fixed test bench.
        auto iamaxOf = [](std::initializer_list<double> xs) {
            std::string data = "data X = {";
            std::string idx  = "data Idx = {";
            int n = 0;
            for (double x : xs) { data += (n ? "," : "") + std::to_string(x); idx += (n ? "," : "") + std::to_string(n); ++n; }
            data += "};\n"; idx += "};\n";
            std::string prog =
                "N = " + std::to_string(n) + ";\n"
                "system ((i,j) | 0 <= i < N, 0 <= j < 1) {\n"
                "  av(i,j)   = av(i,j-1);\n"
                "  idx(i,j)  = idx(i,j-1);\n"
                "  mval(i,j) = select(gt(av(i,j-1), mval(i-1,j)), av(i,j-1), mval(i-1,j));\n"
                "  midx(i,j) = select(gt(av(i,j-1), mval(i-1,j)), idx(i,j-1), midx(i-1,j));\n"
                "}\n"
                "input  X[N]   ((i,j) | 0 <= i < N, j = -1)  : av(i,j)  = abs(X[i]);\n"
                "input  Idx[N] ((i,j) | 0 <= i < N, j = -1)  : idx(i,j) = Idx[i];\n"
                "input         ((i,j) | i = -1, 0 <= j < 1)  : mval(i,j) = -1;\n"
                "input         ((i,j) | i = -1, 0 <= j < 1)  : midx(i,j) = -1;\n"
                "output I[1]   ((i,j) | i = N-1, 0 <= j < 1) : I[0] = midx(i,j);\n"
                + data + idx;
            return runArgmax(parseSureString(prog));
        };
        struct Case { std::initializer_list<double> x; double ref; const char* what; };
        bool cok = true;
        // strict gt => earliest max index on a tie; negatives via |.|; last-element winner
        cok &= std::abs(iamaxOf({  3, -3,  1 }) - 0.0) < 1e-9;   // tie |3|==|-3|: earlier index 0
        cok &= std::abs(iamaxOf({  1,  2,  3, -9 }) - 3.0) < 1e-9; // last element largest
        cok &= std::abs(iamaxOf({ -8,  2,  3 }) - 0.0) < 1e-9;   // magnitude, not signed value
        cok &= std::abs(iamaxOf({  0,  0,  0 }) - 0.0) < 1e-9;   // all zero: first index
        std::cout << "gt/select semantics (ties, sign, last, zeros): " << (cok ? "PASS" : "FAIL") << "\n";
        ok &= cok;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
