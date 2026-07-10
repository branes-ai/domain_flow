// fused_matmul_epilogue.cpp
//
// Issue #1: fused MATMUL output-face epilogue -- bias via the 3rd operand Cin,
// activation via attribute["activation"].  The iteration domain stays the
// matmul (i,j,k) domain; the epilogue is recorded as a Confluence with an
// epilogue on the terminal k = K-1 output face.
//
// Covers the three acceptance criteria:
//   1. a MATMUL node can declare an output-face activation and the elaborated
//      DomainOfComputation records the epilogue on the terminal face
//   2. the attribute (and therefore the epilogue) round-trips through .dfg
//      serialization
//   3. a fused matmul + Cin(bias) + relu example over a small domain,
//      executed numerically through the SURE simulator import path

#include <iostream>
#include <string>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <dfa/sim/dfg_import.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

// arg[2x3] x const[3x2] + bias[2x2], fused activation on the output face
static DomainFlowGraph buildFusedGraph(const std::string& activation) {
    DomainFlowGraph g("fused_matmul_" + activation);

    DomainFlowNode arg(DomainFlowOperator::FUNCTION_ARGUMENT, "func.arg");
    arg.addResult(0, "arg0", "tensor<2x3xf32>");
    auto a = g.addNode(arg);

    DomainFlowNode wgt(DomainFlowOperator::CONSTANT, "tosa.const");
    wgt.addResult(0, "result_0", "tensor<3x2xf32>");
    auto w = g.addNode(wgt);

    DomainFlowNode bias(DomainFlowOperator::CONSTANT, "tosa.const");
    bias.addResult(0, "result_0", "tensor<2x2xf32>");
    auto c = g.addNode(bias);

    DomainFlowNode mm(DomainFlowOperator::MATMUL, "tosa.matmul");
    mm.addOperand(0, "tensor<2x3xf32>");
    mm.addOperand(1, "tensor<3x2xf32>");
    mm.addOperand(2, "tensor<2x2xf32>");             // Cin: bias
    mm.addAttribute("activation", activation);       // fused output-face epilogue
    mm.addResult(0, "result_0", "tensor<2x2xf32>");
    auto m = g.addNode(mm);

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

// criterion 1: the elaborated DoC records the epilogue on the output face
static bool checkElaboration() {
    bool ok = true;

    DomainFlowNode mm(DomainFlowOperator::MATMUL, "tosa.matmul");
    mm.addOperand(0, "tensor<2x3xf32>");
    mm.addOperand(1, "tensor<3x2xf32>");
    mm.addOperand(2, "tensor<2x2xf32>");
    mm.addAttribute("activation", "relu");
    mm.addResult(0, "result_0", "tensor<2x2xf32>");
    mm.instantiateDomain();

    auto confluences = mm.getConfluences();
    std::cout << confluences;
    // A, B, Cin input confluences plus the Cout output confluence
    ok &= (confluences.size() == 4);
    int epilogues = 0;
    for (const auto& conf : confluences) {
        if (conf.hasEpilogue()) {
            ++epilogues;
            ok &= (conf.getEpilogue() == "relu");
            ok &= (conf.getTensorSpec() == "tensor<2x2xf32>");   // Cout on the terminal face
        }
    }
    ok &= (epilogues == 1);
    std::cout << "elaboration records output-face epilogue: " << (ok ? "PASS" : "FAIL") << "\n";

    // a 2-input MATMUL without an activation attribute has no epilogue
    DomainFlowNode plain(DomainFlowOperator::MATMUL, "tosa.matmul");
    plain.addOperand(0, "tensor<2x3xf32>");
    plain.addOperand(1, "tensor<3x2xf32>");
    plain.addResult(0, "result_0", "tensor<2x2xf32>");
    plain.instantiateDomain();
    bool plainOk = true;
    auto plainConfluences = plain.getConfluences();
    plainOk &= (plainConfluences.size() == 3);   // A, B, Cout; no Cin
    for (const auto& conf : plainConfluences) plainOk &= !conf.hasEpilogue();
    std::cout << "plain matmul has no epilogue: " << (plainOk ? "PASS" : "FAIL") << "\n";

    return ok && plainOk;
}

// criteria 2+3: .dfg round-trip preserves the fused epilogue, and the fused
// matmul + Cin(bias) + activation executes correctly through the simulator.
// Leaves fill with 1.0, so A.B = K = 3 and bias adds 1: expect act(4).
static bool checkFused(const std::string& activation, double expected) {
    DomainFlowGraph g = buildFusedGraph(activation);

    // round-trip through a .dfg file
    std::string path = (std::filesystem::temp_directory_path() /
                        ("fused_matmul_epilogue_" + activation + ".dfg")).string();
    g.save(path);
    DomainFlowGraph g2("reloaded");
    g2.load(path);
    std::remove(path.c_str());

    bool ok = true;
    for (const auto& [id, node] : g2.nodes()) {
        if (node.getOperator() == DomainFlowOperator::MATMUL) {
            ok &= (node.getAttribute("activation") == activation);
            // the reloaded node elaborates the same output-face epilogue
            auto reloaded = node;   // instantiateDomain is non-const
            reloaded.instantiateDomain();
            bool found = false;
            for (const auto& conf : reloaded.getConfluences())
                if (conf.hasEpilogue() && conf.getEpilogue() == activation) found = true;
            ok &= found;
        }
    }
    std::cout << activation << " survives .dfg round-trip: " << (ok ? "PASS" : "FAIL") << "\n";

    // numeric execution of the reloaded graph
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
    std::cout << "fused matmul + bias + " << activation << ": " << (numOk ? "PASS" : "FAIL") << "\n";
    return ok && numOk;
}

int main() {
    bool ok = true;

    ok &= checkElaboration();

    // relu(3 + 1) = 4; negate(3 + 1) = -4 proves the epilogue is applied
    ok &= checkFused("relu", 4.0);
    ok &= checkFused("negate", -4.0);

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
