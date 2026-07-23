// systolic_sure.cpp
//
// Jacobi and Gauss-Seidel as PURE SUREs (issue #53) -- docs/SURE/jacobi_systolic.sure and
// gauss_seidel_systolic.sure. Unlike stationary.sure / gauss_seidel.sure (SAREs with affine
// matrix-vector gathers), these route the state cell-to-cell (a PIECEWISE variable xx with
// three equations on i>j / i<j / i=j) and split the reduction so the diagonal solve reads
// adjacent columns -- leaving EVERY dependence a constant displacement. So the RDG is a
// genuine SURE (zero affine arcs), which is the property a regular spatial mapping needs.
//
// This test exercises the piecewise-variable support and checks, for each spec:
//   - the state variable xx has THREE branches (the i>j / i<j / i=j routing)
//   - the RDG is a SURE: every arc is a uniform translation (zero affine arcs)
//   - the iterate still solves A x = b -> the exact [1,1,1]

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>
#include <dfa/sim/rdg.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

static bool checkSpec(const std::string& file, const std::string& op, double convTol) {
    bool ok = true;
    SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/" + file);

    // xx is piecewise: three branches (i>j down, i<j up, i=j solve)
    std::size_t branches = spec.system.equations().at("xx").size();
    bool pw = (branches == 3);
    std::cout << "  " << op << ": xx is piecewise (" << branches << " branches): " << (pw ? "PASS" : "FAIL") << "\n";
    ok &= pw;

    // RDG is a SURE -- zero affine arcs
    Rdg g = buildRdg(spec, op);
    int affine = 0;
    for (const auto& a : g.arcs) if (!a.uniform) ++affine;
    bool sure = !g.isSare() && affine == 0;
    std::cout << "  " << op << ": RDG is a SURE (0 affine of " << g.arcs.size() << " arcs): "
              << (sure ? "PASS" : "FAIL") << "\n";
    ok &= sure;

    // converges to the exact [1,1,1]
    const size_t N = 3;
    const double A[3][3] = { { 10, 1, 1 }, { 1, 10, 1 }, { 1, 1, 10 } };
    const double b[3]    = { 12, 12, 12 };
    double X[3] = {};
    SureSimulator<double> sim(spec.system);
    const SureOutput& out = spec.outputs.front();
    for (const auto& p : out.region.enumerate()) X[out.elemIndex(p)[0]] = out.eval(sim, p);
    double resid = 0;
    for (size_t i = 0; i < N; ++i) {
        double Ax = 0;
        for (size_t j = 0; j < N; ++j) Ax += A[i][j] * X[j];
        resid = std::max(resid, std::abs(Ax - b[i]));
    }
    bool conv = resid < convTol;
    std::cout << "  " << op << ": converged X = [" << X[0] << "," << X[1] << "," << X[2]
              << "]  (||A X - b||_inf = " << resid << "): " << (conv ? "PASS" : "FAIL") << "\n";
    ok &= conv;
    return ok;
}

// piecewise branches of one variable must be disjoint; RecurrenceSystem::add rejects overlaps.
static bool checkOverlapRejected() {
    RecurrenceSystem<double> s;
    auto mk = [](const std::string& n, int lo, int hiEx) {
        Domain d(1); d.axis(0, lo, hiEx);
        return Equation<double>{ n, d, {},
            [](const std::vector<double>&, const IndexPoint&) { return 0.0; },
            [](const IndexPoint&) { return 0.0; } };
    };
    s.add(mk("v", 0, 5));                 // 0..4
    bool threw = false;
    try { s.add(mk("v", 3, 8)); }         // 3..7 -- overlaps 3,4
    catch (const std::exception&) { threw = true; }
    std::cout << "  overlapping branch domains are rejected: " << (threw ? "PASS" : "FAIL") << "\n";
    return threw;
}

int main() {
    bool ok = true;
    try {
        ok &= checkSpec("jacobi_systolic.sure", "jacobi_systolic", 1e-3);        // Jacobi: slower
        ok &= checkSpec("gauss_seidel_systolic.sure", "gauss_seidel_systolic", 1e-6);  // GS: exact by K=8
        ok &= checkOverlapRejected();
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }
    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
