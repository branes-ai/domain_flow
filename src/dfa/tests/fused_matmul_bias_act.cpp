// fused_matmul_bias_act.cpp
//
// Issue #2: dedicated FUSED_MATMUL_BIAS_ACT operator with fused domain
// elaboration:  Y = activation(A*B + bias)  as a *single* (i,j,k) domain.
// The matmul accumulates C(i,j,k) = C(i,j,k-1) + A(i,k)*B(k,j); the epilogue
// (bias-add + activation) is a boundary recurrence on the terminal k = K-1
// face, where the bias enters and Y leaves.  No intermediate tensor is
// materialized.
//
// Covers the acceptance criteria:
//   1. FUSED_MATMUL_BIAS_ACT elaborates a single (i,j,k) domain with the
//      epilogue recorded on the k = K-1 face (bias input confluence and the
//      Y output confluence with activation epilogue share the terminal face)
//   2. .dfg round-trip; worked example vs a reference through the simulator
//   3. schedule application produces correct wavefronts under an
//      output-stationary tau = [1,1,1]

#include <iostream>
#include <string>
#include <cmath>
#include <cstdio>
#include <limits>
#include <filesystem>
#include <dfa/sim/dfg_import.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

static DomainFlowNode makeFusedNode(const std::string& activation) {
    DomainFlowNode mm(DomainFlowOperator::FUSED_MATMUL_BIAS_ACT, "dfa.fused_matmul_bias_act");
    mm.addOperand(0, "tensor<2x3xf32>");   // A [M,K]
    mm.addOperand(1, "tensor<3x2xf32>");   // B [K,N]
    mm.addOperand(2, "tensor<2x2xf32>");   // bias, broadcast on the (i,j) face
    mm.addAttribute("activation", activation);
    mm.addResult(0, "result_0", "tensor<2x2xf32>");
    return mm;
}

static DomainFlowGraph buildFusedGraph(const std::string& activation) {
    DomainFlowGraph g("fused_mlp_layer_" + activation);

    DomainFlowNode arg(DomainFlowOperator::FUNCTION_ARGUMENT, "func.arg");
    arg.addResult(0, "arg0", "tensor<2x3xf32>");
    auto a = g.addNode(arg);

    DomainFlowNode wgt(DomainFlowOperator::CONSTANT, "tosa.const");
    wgt.addResult(0, "result_0", "tensor<3x2xf32>");
    auto w = g.addNode(wgt);

    DomainFlowNode bias(DomainFlowOperator::CONSTANT, "tosa.const");
    bias.addResult(0, "result_0", "tensor<2x2xf32>");
    auto c = g.addNode(bias);

    auto m = g.addNode(makeFusedNode(activation));

    DomainFlowNode ret(DomainFlowOperator::FUNCTION_RETURN, "func.return");
    ret.addOperand(0, "tensor<2x2xf32>");
    ret.addResult(0, "result0", "tensor<2x2xf32>");
    auto r = g.addNode(ret);

    DomainFlowEdge e;
    g.addEdge(a, 0, m, 0, e);
    g.addEdge(w, 0, m, 1, e);
    g.addEdge(c, 0, m, 2, e);
    g.addEdge(m, 0, r, 0, e);
    g.addSource(a); g.addSource(w); g.addSource(c); g.addSink(r);
    return g;
}

// criterion 1: single (i,j,k) domain, epilogue on the terminal face
static bool checkElaboration() {
    bool ok = true;

    DomainFlowNode mm = makeFusedNode("relu");
    mm.instantiateDomain();

    auto confluences = mm.getConfluences();
    std::cout << confluences;
    // A, B, bias input confluences plus the Y output confluence
    ok &= (confluences.size() == 4);

    // exactly one output confluence, carrying the activation epilogue
    std::size_t terminalFace = static_cast<std::size_t>(-1);
    int epilogues = 0;
    for (const auto& conf : confluences) {
        if (conf.hasEpilogue()) {
            ++epilogues;
            ok &= (conf.getEpilogue() == "relu");
            terminalFace = conf.getFaceId();
        }
    }
    ok &= (epilogues == 1);

    // the bias input confluence shares the terminal face with Y: the epilogue
    // is a boundary recurrence on the k = K-1 face, not an accumulator seed
    int biasOnTerminalFace = 0;
    for (const auto& conf : confluences) {
        if (!conf.hasEpilogue() && conf.getFaceId() == terminalFace) {
            ++biasOnTerminalFace;
            ok &= (conf.getTensorSpec() == "tensor<2x2xf32>");
        }
    }
    ok &= (biasOnTerminalFace == 1);

    std::cout << "fused elaboration (epilogue + bias on terminal face): " << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

// criterion 3: correct wavefronts under an output-stationary tau = [1,1,1]
static bool checkSchedule() {
    bool ok = true;

    DomainFlowNode mm = makeFusedNode("relu");
    mm.instantiateDomain();
    mm.instantiateIndexSpace();

    const auto& points = mm.doc.getIndexSpace().getPoints();
    ok &= !points.empty();

    ScheduleVector<int> tau;
    tau = { 1, 1, 1 };   // output-stationary: C(i,j) accumulates in place along k
    mm.applyLinearSchedule(tau);

    // expected wavefronts are the level sets of t = i + j + k over the index space
    int tmin = std::numeric_limits<int>::max(), tmax = std::numeric_limits<int>::min();
    for (const auto& p : points) {
        int t = p[0] + p[1] + p[2];
        tmin = std::min(tmin, t);
        tmax = std::max(tmax, t);
    }
    ok &= (mm.schedule.calculateLatency() == static_cast<std::uint64_t>(tmax - tmin + 1));

    std::size_t activities = 0;
    for (const auto& [t, wf] : mm.schedule) {
        activities += wf.size();
        for (std::size_t i = 0; i < wf.size(); ++i) {
            const IndexPoint& p = wf[i];
            ok &= (static_cast<std::size_t>(p[0] + p[1] + p[2]) == t);   // on its wavefront
        }
    }
    ok &= (activities == points.size());   // every index point scheduled exactly once

    std::cout << "output-stationary tau=[1,1,1]: latency=" << mm.schedule.calculateLatency()
              << " activities=" << activities << " over " << points.size() << " points: "
              << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

// criterion 2: .dfg round-trip and numeric execution vs reference.
// Leaves fill with 1.0, so A.B = K = 3 and bias adds 1: expect act(4).
static bool checkFused(const std::string& activation, double expected) {
    DomainFlowGraph g = buildFusedGraph(activation);

    std::string path = (std::filesystem::temp_directory_path() /
                        ("fused_matmul_bias_act_" + activation + ".dfg")).string();
    g.save(path);
    DomainFlowGraph g2("reloaded");
    g2.load(path);
    std::remove(path.c_str());

    bool ok = true;
    bool foundOp = false;
    for (const auto& [id, node] : g2.nodes()) {
        if (node.getOperator() == DomainFlowOperator::FUSED_MATMUL_BIAS_ACT) {
            foundOp = true;   // the operator enum itself round-tripped
            ok &= (node.getAttribute("activation") == activation);
        }
    }
    ok &= foundOp;
    std::cout << "FUSED_MATMUL_BIAS_ACT(" << activation << ") survives .dfg round-trip: "
              << (ok ? "PASS" : "FAIL") << "\n";

    ImportResult ir = importDomainFlowGraph(g2);
    if (ir.outputs.empty()) { std::cout << "  NO OUTPUTS\n"; return false; }
    SureSimulator<double> sim(ir.system);
    const GraphOutput& out = ir.outputs.front();
    bool numOk = true;
    for (int i = 0; i < out.shape[0]; ++i)
        for (int j = 0; j < out.shape[1]; ++j) {
            double got = sim.eval(out.var, IndexPoint({ i, j }));
            bool match = std::abs(got - expected) < 1e-9;
            std::cout << "  " << out.var << "[" << i << "][" << j << "] = " << got
                      << "  (ref " << expected << ")" << (match ? "" : "  MISMATCH") << "\n";
            numOk &= match;
        }
    std::cout << "fused Y = " << activation << "(A*B + bias): " << (numOk ? "PASS" : "FAIL") << "\n";
    return ok && numOk;
}

int main() {
    bool ok = true;

    ok &= checkElaboration();
    ok &= checkSchedule();

    // relu(3 + 1) = 4; negate(3 + 1) = -4 proves the epilogue is applied
    ok &= checkFused("relu", 4.0);
    ok &= checkFused("negate", -4.0);

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
