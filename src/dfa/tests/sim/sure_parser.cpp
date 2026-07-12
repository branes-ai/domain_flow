// sure_parser.cpp
//
// Issues #15/#17: execute the docs/SURE DSL directly, v2 confluence form.
// Data enters and leaves the domain through symmetric input/output
// declarations: equality-pinned face regions with derived outward normals.
// Checks:
//   - numeric output of docs/SURE/matmul.sure against a reference C = A*B
//   - the derived confluence structure (face normals for A, B, seed, C)
//   - schedule analysis on the parsed system (free legal, tau=[1,1,1] legal,
//     tau=[1,1,0] ILLEGAL on the k-accumulation)
//   - a non-axis-aligned (diagonal equality) input face, and negative bounds
//   - the validation contracts: coverage, face well-formedness, flux
//     consistency, element range, data binding -- with line-numbered
//     diagnostics

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
    ok &= (spec.inputs.size() == 3);

    // derived confluence orientation: outward normals of the declared faces
    bool nok = true;
    for (const auto& in : spec.inputs) {
        if (in.tensor == "A")      nok &= (in.normal == std::vector<int>{ 0, -1, 0 });
        else if (in.tensor == "B") nok &= (in.normal == std::vector<int>{ -1, 0, 0 });
        else                       nok &= (in.normal == std::vector<int>{ 0, 0, -1 });  // seed
    }
    nok &= (spec.outputs.front().normal == std::vector<int>{ 0, 0, 1 });
    std::cout << "derived face normals (A,B,seed,C): " << (nok ? "PASS" : "FAIL") << "\n";
    ok &= nok;

    // reference C = A*B
    const double A[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } };
    const double B[3][2] = { { 7, 8 }, { 9, 10 }, { 11, 12 } };

    SureSimulator<double> sim(spec.system);
    const SureOutput& out = spec.outputs.front();
    int checked = 0;
    for (const auto& p : out.region.enumerate()) {
        std::vector<long> idx = out.elemIndex(p);
        double got = out.eval(sim, p);
        double ref = 0;
        for (int k = 0; k < 3; ++k) ref += A[idx[0]][k] * B[k][idx[1]];
        bool m = std::abs(got - ref) < 1e-9;
        std::cout << "  C[" << idx[0] << "][" << idx[1] << "] = " << got
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

    // an overriding schedule must revalidate flux: tau=[1,1,-1] reverses the
    // output face flow even though it is caught here before legality
    bool fok = false;
    try { validateSureFlux(spec, { 1, 1, -1 }); }
    catch (const std::exception& e) { fok = std::string(e.what()).find("flux") != std::string::npos; }
    std::cout << "flux revalidation for overridden tau: " << (fok ? "PASS" : "FAIL") << "\n";
    ok &= fok;

    // memory analysis + eviction run agree
    LivenessReport lr = sim2.analyzeMemory(good);
    long peak = 0;
    sim2.run(good, &peak);
    bool mok = (peak == lr.peakLiveValues);
    std::cout << "memory analysis vs eviction run: " << (mok ? "PASS" : "FAIL") << "\n";

    return ok && sok && mok;
}

// triangular constraint (k <= j), a NON-AXIS-ALIGNED input face pinned by the
// diagonal equality j - k = -1 (where the w-propagation actually exits the
// domain), and a diagonal output face k - j = 0
static bool checkConstraints() {
    const char* prog = R"(
N = 3;
system ((j,k) | 0 <= j < N, 0 <= k < N, k <= j) {
    s(j,k) = s(j,k-1) + w(j,k);
    w(j,k) = w(j-1,k);
}
input W[3]  ((j,k) | -1 <= j < 2, 0 <= k < N, j - k = -1) : w(j,k) = W[k];
input       ((j,k) | 0 <= j < N, k = -1)                  : s(j,k) = 0;
output S[3] ((j,k) | 0 <= j < N, 0 <= k < N, k - j = 0)   : S[j] = s(j,k);
data W = { 10, 20, 30 };
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

    // diagonal input face: plane j - k = -1, domain on the j - k > -1 side
    // => outward normal (-1, 1)
    bool found = false;
    for (const auto& in : spec.inputs)
        if (in.tensor == "W") { found = true; ok &= (in.normal == std::vector<int>{ -1, 1 }); }
    ok &= found;

    // S[j] = s(j,j) = sum_{k=0..j} W[k]  (prefix sums: 10, 30, 60)
    SureSimulator<double> sim(spec.system);
    const double W[3] = { 10, 20, 30 };
    const SureOutput& out = spec.outputs.front();
    int outs = 0;
    for (const auto& p : out.region.enumerate()) {
        std::vector<long> idx = out.elemIndex(p);
        double ref = 0;
        for (long k = 0; k <= idx[0]; ++k) ref += W[k];
        ok &= (std::abs(out.eval(sim, p) - ref) < 1e-9);
        ++outs;
    }
    ok &= (outs == 3);
    std::cout << "triangular domain + diagonal faces: " << (ok ? "PASS" : "FAIL") << "\n";

    // negative domain bounds (conv2d-style): u(k) = u(k-1) + 1 over -1 <= k < 2
    const char* prog2 = R"(
system ((k) | -1 <= k < 2) { u(k) = u(k-1) + 1; }
input       ((k) | k = -2) : u(k) = 0;
output U[1] ((k) | k = 1)  : U[0] = u(k);
)";
    SureSpec spec2 = parseSureString(prog2);
    SureSimulator<double> sim2(spec2.system);
    // u(-1)=1, u(0)=2, u(1)=3: value 3 proves the k=-1 point is in the domain
    double got = spec2.outputs.front().eval(sim2, IndexPoint({ 1 }));
    bool nok = std::abs(got - 3.0) < 1e-9;
    std::cout << "negative domain bounds: " << (nok ? "PASS" : "FAIL")
              << "  (u(1) = " << got << ", ref 3)\n";

    return ok && nok;
}

// docs/SURE/qr.sure: Modified Gram-Schmidt over one shared triangular domain;
// verify the factorization A = Q*R against the classic 3x3 example
static bool checkQr() {
    SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/qr.sure");
    bool ok = true;

    const double A[3][3] = { { 12, -51, 4 }, { 6, 167, -68 }, { -4, 24, -41 } };
    double Q[3][3] = {};
    double R[3][3] = {};

    SureSimulator<double> sim(spec.system);
    for (const auto& out : spec.outputs) {
        for (const auto& fp : out.region.enumerate()) {
            std::vector<long> idx = out.elemIndex(fp);
            double v = out.eval(sim, fp);
            if (out.tensor == "Q")          Q[idx[0]][idx[1]] = v;
            else if (out.tensor == "Rdiag") R[idx[0]][idx[0]] = v;
            else                            R[idx[0]][idx[1]] = v;   // Roff
        }
    }

    // exact factors for this A: R = [[14,21,-14],[0,175,-70],[0,0,35]]
    const double Rref[3][3] = { { 14, 21, -14 }, { 0, 175, -70 }, { 0, 0, 35 } };
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            ok &= (std::abs(R[r][c] - Rref[r][c]) < 1e-9);

    // Q orthonormal and Q*R == A
    double maxOrtho = 0, maxQR = 0;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) {
            double dot = 0, qr = 0;
            for (int t = 0; t < 3; ++t) { dot += Q[t][r] * Q[t][c]; qr += Q[r][t] * R[t][c]; }
            maxOrtho = std::max(maxOrtho, std::abs(dot - (r == c ? 1.0 : 0.0)));
            maxQR = std::max(maxQR, std::abs(qr - A[r][c]));
        }
    ok &= (maxOrtho < 1e-9) && (maxQR < 1e-9);
    std::cout << "qr.sure: max|Q^T*Q - I| = " << maxOrtho << "  max|Q*R - A| = " << maxQR
              << "  R exact: " << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

// docs/SURE/conv2d.sure: 3x3 Sobel-x over a 4x4 ramp with [1,1] zero padding;
// verify against a direct correlation reference
static bool checkConv2d() {
    SureSpec spec = parseSureFile(std::string(SURE_DOCS_DIR) + "/conv2d.sure");
    bool ok = true;

    const int N = 4;
    double I[4][4];
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c) I[r][c] = 1.0 + r * N + c;
    const double W[3][3] = { { 1, 0, -1 }, { 2, 0, -2 }, { 1, 0, -1 } };
    auto ipad = [&](int r, int c) {
        return (r < 0 || r >= N || c < 0 || c >= N) ? 0.0 : I[r][c];
    };

    SureSimulator<double> sim(spec.system);
    const SureOutput& out = spec.outputs.front();
    int checked = 0;
    for (const auto& fp : out.region.enumerate()) {
        std::vector<long> idx = out.elemIndex(fp);
        double got = out.eval(sim, fp);
        double ref = 0;
        for (int k = -1; k <= 1; ++k)
            for (int l = -1; l <= 1; ++l)
                ref += ipad(static_cast<int>(idx[0]) + k, static_cast<int>(idx[1]) + l) * W[k + 1][l + 1];
        ok &= (std::abs(got - ref) < 1e-9);
        ++checked;
    }
    ok &= (checked == N * N);
    std::cout << "conv2d.sure == direct correlation reference (" << checked << " points): "
              << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

static bool expectParseError(const char* what, const std::string& prog, const char* needle) {
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
    // a minimal valid program to mutate
    const std::string sys =
        "N = 2;\n"
        "system ((i,j) | 0 <= i,j < N) { c(i,j) = c(i,j-1); }\n";
    const std::string seed =
        "input ((i,j) | 0 <= i < N, j = -1) : c(i,j) = 0;\n";
    const std::string outp =
        "output C[2][2] ((i,j) | 0 <= i < N, j = 1) : C[i][j] = c(i,j);\n";

    bool ok = true;
    // face well-formedness
    ok &= expectParseError("face without an equality",
        sys + "input ((i,j) | 0 <= i < N, -2 <= j < 0) : c(i,j) = 0;\n" + outp,
        "exactly one equality");
    ok &= expectParseError("face plane cuts the domain",
        sys + "input ((i,j) | 0 <= i < N, 0 <= j < N, i + j = 1) : c(i,j) = 0;\n" + outp,
        "cuts the domain interior");
    // coverage
    ok &= expectParseError("uncovered boundary access",
        sys + outp,
        "not covered by any input face");
    // flux consistency (output face at i = 1 so input/output planes differ)
    const std::string outpI =
        "output C[2][2] ((i,j) | i = 1, 0 <= j < N) : C[i][j] = c(i,j);\n";
    ok &= expectParseError("output face with backward flux",
        sys + seed + outpI + "tau = [-1, 1];\n",
        "non-positive flux");
    ok &= expectParseError("input face with forward flux",
        sys + seed + outp + "tau = [1, -1];\n",
        "non-negative flux");
    // element range
    ok &= expectParseError("output element out of range",
        sys + seed +
        "output C[1][1] ((i,j) | 0 <= i < N, j = 1) : C[i][j] = c(i,j);\n",
        "out of range");
    // data binding
    ok &= expectParseError("tensor without data",
        sys +
        "input W[2] ((i,j) | 0 <= i < N, j = -1) : c(i,j) = W[i];\n" + outp,
        "no data binding");
    // expression rules
    ok &= expectParseError("non-affine tap index",
        "N = 2;\n"
        "system ((i,j) | 0 <= i,j < N) { c(i,j) = c(i*j, 0); }\n" + seed + outp,
        "non-affine");
    ok &= expectParseError("tensor read in an equation body",
        "N = 2;\n"
        "system ((i,j) | 0 <= i,j < N) { c(i,j) = c(i,j-1) + W[i]; }\n" + seed + outp,
        "input/output expressions");
    ok &= expectParseError("input before the system block",
        "N = 2;\n"
        "input W[2] ((i,j) | 0 <= i < N, j = -1) : c(i,j) = W[i];\n",
        "before the system");
    ok &= expectParseError("input expression taps a recurrence variable",
        sys + "input ((i,j) | 0 <= i < N, j = -1) : c(i,j) = c(i,j);\n" + outp,
        "may not read recurrence variables");
    ok &= expectParseError("missing output",
        sys + seed,
        "no output");
    // review round (PR #18): supporting-hyperplane and binding contracts
    ok &= expectParseError("face plane splits domain points",
        "N = 3;\n"
        "system ((i,j) | 0 <= i,j < N) { c(i,j) = c(i,j-1); }\n"
        "input ((i,j) | 0 <= i < N, 0 <= j < N, i + j = 1) : c(i,j) = 0;\n"
        "input ((i,j) | 0 <= i < N, j = -1) : c(i,j) = 0;\n"
        "output C[3][3] ((i,j) | 0 <= i < N, j = 2) : C[i][j] = c(i,j);\n",
        "cuts the domain interior");
    ok &= expectParseError("constant face equality",
        sys + "input ((i,j) | 0 <= i < N, 0 = 1) : c(i,j) = 0;\n" + outp,
        "no index terms");
    ok &= expectParseError("named input must read its tensor",
        sys + "input A[2] ((i,j) | 0 <= i < N, j = -1) : c(i,j) = 0;\n" + outp,
        "must read its declared tensor");
    ok &= expectParseError("input face reading two tensors",
        sys +
        "input W[2] ((i,j) | 0 <= i < N, j = -1) : c(i,j) = W[i];\n"
        "data W = { 1, 2 };\n"
        "input V[2] ((i,j) | 0 <= i < N, j = 2) : c(i,j) = V[0] + W[0];\n"
        "data V = { 1, 2 };\n" + outp,
        "may only read its declared tensor");
    ok &= expectParseError("undeclared tap source",
        "N = 2;\n"
        "system ((i,j) | 0 <= i,j < N) { c(i,j) = typo(i,j); }\n" + seed + outp,
        "unknown recurrence variable");
    return ok;
}

int main() {
    bool ok = true;
    try {
        ok &= checkMatmul();
        ok &= checkQr();
        ok &= checkConv2d();
        ok &= checkConstraints();
        ok &= checkDiagnostics();
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        ok = false;
    }
    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
