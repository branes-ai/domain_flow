// rdg_sure.cpp
//
// Reduced Dependency Graph (RDG) of a SURE/SARE (issue #103). The RDG collapses the
// unbounded expanded dependence graph to one node per recurrence variable and one arc
// per dependence, each arc carrying its affine map (A, b): uniform (A = I, a
// translation theta = -b) or affine (A != I, a projection/broadcast). A single affine
// arc makes the system a SARE.
//
// This test parses the executable catalog specs (the single source of truth) and
// checks that buildRdg classifies them the way the derivations claim:
//   - gemv is a genuine SURE  (every arc a uniform translation)
//   - lu / cholesky / ldlt are SAREs (the affine pivot taps), 3 affine arcs each
//   - trsolve is a SARE with exactly one affine arc (the diagonal x(j,j) broadcast)
//   - a specific uniform arc has theta = -b (acc(i,j,k) = acc(i,j-1,k) -> theta=(0,1,0))
//   - a specific affine arc reconstructs the pivot tap lu reads: a(k,k,k-1)

#include <iostream>
#include <string>
#include <vector>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/rdg.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

static Rdg rdgOf(const std::string& op) {
    return buildRdg(parseSureFile(std::string(SURE_DOCS_DIR) + "/" + op + ".sure"), op);
}
static int affineCount(const Rdg& g) {
    int n = 0;
    for (const auto& a : g.arcs) if (!a.uniform) ++n;
    return n;
}

int main() {
    bool ok = true;
    try {
        // ---- SURE vs SARE classification across a representative slice ----
        Rdg gemv = rdgOf("gemv");
        bool c1 = (!gemv.isSare()) && (affineCount(gemv) == 0) && !gemv.arcs.empty();
        std::cout << "gemv is a SURE (all arcs uniform): " << (c1 ? "PASS" : "FAIL")
                  << "  (" << gemv.variables.size() << " nodes, " << gemv.arcs.size() << " arcs)\n";
        ok &= c1;

        for (const char* op : { "lu", "cholesky", "ldlt" }) {
            Rdg g = rdgOf(op);
            bool c = g.isSare() && (affineCount(g) == 3);
            std::cout << "  " << op << ": SARE with 3 affine pivot arcs: " << (c ? "PASS" : "FAIL")
                      << "  (affine=" << affineCount(g) << ")\n";
            ok &= c;
        }

        Rdg trs = rdgOf("trsolve");
        bool c2 = trs.isSare() && (affineCount(trs) == 1);
        std::cout << "trsolve is a SARE with 1 affine arc (diagonal broadcast): "
                  << (c2 ? "PASS" : "FAIL") << "  (affine=" << affineCount(trs) << ")\n";
        ok &= c2;

        // ---- a uniform arc carries theta = -b : gemv acc(i,j,k)=acc(i,j-1,k) -> (0,1,0) ----
        bool foundTheta = false;
        for (const auto& a : gemv.arcs) {
            if (a.from == "acc" && a.to == "acc") {
                foundTheta = a.uniform && (a.theta == std::vector<int>{ 0, 1, 0 })
                          && (a.b == std::vector<int>{ 0, -1, 0 });
                break;
            }
        }
        std::cout << "uniform self-loop acc->acc has theta=(0,1,0), b=(0,-1,0): "
                  << (foundTheta ? "PASS" : "FAIL") << "\n";
        ok &= foundTheta;

        // ---- an affine arc reconstructs lu's pivot tap a(k,k,k-1):
        //      A projects (i,j,k) -> (k,k,k-1), i.e. every row = (0,0,1), b = (0,0,-1) ----
        Rdg lu = rdgOf("lu");
        bool foundPivot = false;
        const std::vector<std::vector<int>> pivotA = { { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 } };
        for (const auto& a : lu.arcs)
            if (!a.uniform && a.A == pivotA && a.b == std::vector<int>{ 0, 0, -1 }) { foundPivot = true; break; }
        std::cout << "lu has the affine pivot tap a(k,k,k-1) [A projects to (k,k,k-1)]: "
                  << (foundPivot ? "PASS" : "FAIL") << "\n";
        ok &= foundPivot;

        // ---- every arc endpoint is a declared node (no dangling arcs) ----
        bool closed = true;
        for (const auto& a : lu.arcs) {
            bool hasFrom = false, hasTo = false;
            for (const auto& v : lu.variables) { hasFrom |= (v == a.from); hasTo |= (v == a.to); }
            closed &= hasFrom && hasTo;
        }
        std::cout << "all arc endpoints are graph nodes: " << (closed ? "PASS" : "FAIL") << "\n";
        ok &= closed;
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
