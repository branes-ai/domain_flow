// sure_parser.cpp
//
// Issue #15: execute the docs/SURE DSL directly.  Parses the canonical
// systolic matmul SURE (docs/SURE/matmul.md notation) from text, runs it
// through the simulator, and checks:
//   - numeric output against a reference C = A*B
//   - schedule analysis on the parsed system (free legal, tau=[1,1,1] legal,
//     tau=[1,1,0] ILLEGAL on the k-accumulation)
//   - triangular (multi-var) constraints and negative domain bounds
//   - line-numbered diagnostics for malformed programs

#include <iostream>
#include <string>
#include <cmath>
#include <dfa/sim/sure_parser.hpp>
#include <dfa/sim/legality.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

// The canonical example is docs/SURE/matmul.sure -- loaded from the source
// tree (SURE_DOCS_DIR is set by CMake) so the executable doc is the single
// source of truth exercised by this test.

static bool checkMatmul() {
    SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/matmul.sure");
    bool ok = true;

    ok &= (spec.indexNames == std::vector<std::string>{ "i", "j", "k" });
    ok &= (spec.tau == std::vector<int>{ 1, 1, 1 });
    ok &= (spec.outputs.size() == 1);

    // reference C = A*B
    const double A[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } };
    const double B[3][2] = { { 7, 8 }, { 9, 10 }, { 11, 12 } };

    SureSimulator<double> sim(spec.system);
    const SureOutput& out = spec.outputs.front();
    int checked = 0;
    for (const auto& p : out.domain.enumerate()) {
        double got = sim.eval(out.var, out.map.apply(p));
        double ref = 0;
        for (int k = 0; k < 3; ++k) ref += A[p[0]][k] * B[k][p[1]];
        bool m = std::abs(got - ref) < 1e-9;
        std::cout << "  c(" << p[0] << "," << p[1] << ") = " << got
                  << "  (ref " << ref << ")" << (m ? "" : "  MISMATCH") << "\n";
        ok &= m;
        ++checked;
    }
    ok &= (checked == 4);
    std::cout << "parsed matmul == A*B reference: " << (ok ? "PASS" : "FAIL") << "\n";

    // schedule analysis on the parsed system
    SureSimulator<double> sim2(spec.system);
    ExplicitSchedule freeSched = sim2.computeFreeSchedule();
    LinearSchedule good(spec.tau);
    LinearSchedule bad({ 1, 1, 0 });
    bool sok = true;
    sok &=  checkLegality(spec.system, freeSched).legal;
    sok &=  checkLegality(spec.system, good).legal;
    sok &= !checkLegality(spec.system, bad).legal;
    std::cout << "  free: " << checkLegality(spec.system, freeSched) << "\n";
    std::cout << "  tau=[1,1,1]: " << checkLegality(spec.system, good) << "\n";
    std::cout << "  tau=[1,1,0]: " << checkLegality(spec.system, bad) << "\n";
    std::cout << "schedule analysis on parsed system: " << (sok ? "PASS" : "FAIL") << "\n";

    // memory analysis + eviction run agree
    LivenessReport lr = sim2.analyzeMemory(good);
    long peak = 0;
    sim2.run(good, &peak);
    bool mok = (peak == lr.peakLiveValues);
    std::cout << "memory analysis vs eviction run: " << (mok ? "PASS" : "FAIL") << "\n";

    return ok && sok && mok;
}

// triangular constraint (k <= j via multi-var HalfSpace), affine tap
// projection, and a boundary reading a 1D input
static bool checkConstraints() {
    const char* prog = R"(
N = 3;
input W[3] = { 10, 20, 30 };
system ((j,k) | 0 <= j < N, 0 <= k < N, k <= j) {
    s(j,k) = s(j,k-1) + w(j,k);
    w(j,k) = w(j-1,k);
}
boundary s(j,k) = 0;
boundary w(j,k) = W[k];
output ((j) | 0 <= j < N) s(j, j);
)";
    SureSpec spec = parseSureString(prog);
    bool ok = true;

    // domain respects the triangular constraint: enumerate and verify k <= j
    const auto& eq = spec.system.at("s");
    int points = 0;
    for (const auto& p : eq.domain.enumerate()) {
        ok &= (p[1] <= p[0]);
        ++points;
    }
    ok &= (points == 6);   // 1 + 2 + 3 points in the lower triangle

    // s(j,j) = sum_{k=0..j} W[k]  (prefix sums: 10, 30, 60)
    SureSimulator<double> sim(spec.system);
    const double W[3] = { 10, 20, 30 };
    for (int j = 0; j < 3; ++j) {
        double ref = 0;
        for (int k = 0; k <= j; ++k) ref += W[k];
        double got = sim.eval("s", IndexPoint({ j, j }));
        ok &= (std::abs(got - ref) < 1e-9);
    }
    std::cout << "triangular domain + input boundary: " << (ok ? "PASS" : "FAIL") << "\n";

    // negative domain bounds (conv2d-style): u(k) = u(k-1) + 1 over -1 <= k < 2
    const char* prog2 = R"(
system ((k) | -1 <= k < 2) { u(k) = u(k-1) + 1; }
boundary u(k) = 0;
output ((k) | 1 <= k < 2) u(k);
)";
    SureSpec spec2 = parseSureString(prog2);
    SureSimulator<double> sim2(spec2.system);
    // u(-1)=1, u(0)=2, u(1)=3: value 3 proves the k=-1 point is in the domain
    double got = sim2.eval("u", IndexPoint({ 1 }));
    bool nok = std::abs(got - 3.0) < 1e-9;
    std::cout << "negative domain bounds: " << (nok ? "PASS" : "FAIL")
              << "  (u(1) = " << got << ", ref 3)\n";

    return ok && nok;
}

static bool expectParseError(const char* what, const char* prog, const char* needle) {
    try {
        SureSpec spec = parseSureString(prog);
        std::cout << what << ": FAIL (no diagnostic)\n";
        return false;
    } catch (const std::exception& e) {
        bool ok = std::string(e.what()).find(needle) != std::string::npos &&
                  std::string(e.what()).find("line") != std::string::npos;
        std::cout << what << ": " << (ok ? "PASS" : "FAIL") << "  [" << e.what() << "]\n";
        return ok;
    }
}

static bool checkDiagnostics() {
    bool ok = true;
    ok &= expectParseError("non-affine tap index",
        "N = 2;\n"
        "system ((i,j) | 0 <= i,j < N) { c(i,j) = c(i*j, 0); }\n"
        "output ((i) | 0 <= i < N) c(i, 0);\n",
        "non-affine");
    ok &= expectParseError("unknown input array",
        "N = 2;\n"
        "system ((i,j) | 0 <= i,j < N) { c(i,j) = c(i-1,j); }\n"
        "boundary c(i,j) = Q[i];\n"
        "output ((i) | 0 <= i < N) c(i, 0);\n",
        "unknown input array");
    ok &= expectParseError("unbounded index",
        "N = 2;\n"
        "system ((i,j) | 0 <= i < N, 0 <= j) { c(i,j) = c(i-1,j); }\n"
        "output ((i) | 0 <= i < N) c(i, 0);\n",
        "no constant lower and upper bound");
    ok &= expectParseError("boundary taps a recurrence variable",
        "N = 2;\n"
        "system ((i,j) | 0 <= i,j < N) { c(i,j) = c(i-1,j); d(i,j) = d(i,j-1); }\n"
        "boundary c(i,j) = d(i,j);\n"
        "output ((i) | 0 <= i < N) c(i, 0);\n",
        "boundary expressions may not read recurrence variables");
    ok &= expectParseError("array read in an equation body",
        "N = 2;\n"
        "input W[2] = { 1, 2 };\n"
        "system ((i,j) | 0 <= i,j < N) { c(i,j) = c(i-1,j) + W[i]; }\n"
        "output ((i) | 0 <= i < N) c(i, 0);\n",
        "boundary expressions");
    ok &= expectParseError("missing output",
        "N = 2;\n"
        "system ((i,j) | 0 <= i,j < N) { c(i,j) = c(i-1,j); }\n",
        "no output");
    return ok;
}

int main() {
    bool ok = true;
    try {
        ok &= checkMatmul();
        ok &= checkConstraints();
        ok &= checkDiagnostics();
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }
    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
