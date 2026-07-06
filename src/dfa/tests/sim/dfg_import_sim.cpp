// dfg_import_sim.cpp
//
// Build a small DomainFlowGraph via the graph API, lower it to a RecurrenceSystem
// with the .dfg importer, execute it, and verify numeric output.  Then round-trip
// the graph through save()/load() to exercise the on-disk .dfg import path.
//
// Graph:  arg[2x3] and const[3x2] feed MATMUL, whose result feeds RETURN[2x2].
// Leaf inputs fill with 1.0, so C[i][j] = sum_k 1*1 = K = 3 for every (i,j).

#include <iostream>
#include <string>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <dfa/sim/dfg_import.hpp>

using namespace sw::dfa;
using namespace sw::dfa::sim;

static DomainFlowGraph buildGraph() {
    DomainFlowGraph g("synthetic_matmul");

    DomainFlowNode arg(DomainFlowOperator::FUNCTION_ARGUMENT, "func.arg");
    arg.addResult(0, "arg0", "tensor<2x3xf32>");
    auto a = g.addNode(arg);

    DomainFlowNode cst(DomainFlowOperator::CONSTANT, "tosa.const");
    cst.addResult(0, "result_0", "tensor<3x2xf32>");
    auto b = g.addNode(cst);

    DomainFlowNode mm(DomainFlowOperator::MATMUL, "tosa.matmul");
    mm.addOperand(0, "tensor<2x3xf32>");
    mm.addOperand(1, "tensor<3x2xf32>");
    mm.addResult(0, "result_0", "tensor<2x2xf32>");
    auto m = g.addNode(mm);

    DomainFlowNode ret(DomainFlowOperator::FUNCTION_RETURN, "func.return");
    ret.addOperand(0, "tensor<2x2xf32>");
    ret.addResult(0, "result0", "tensor<2x2xf32>");
    auto r = g.addNode(ret);

    DomainFlowEdge e;
    g.addEdge(a, 0, m, 0, e);   // arg   -> matmul input 0
    g.addEdge(b, 0, m, 1, e);   // const -> matmul input 1
    g.addEdge(m, 0, r, 0, e);   // matmul -> return
    g.addSource(a); g.addSource(b); g.addSink(r);
    return g;
}

static bool checkResult(ImportResult& ir, const char* tag) {
    std::cout << "[" << tag << "] " << ir.report;
    std::cout << "  indexSpaceSize=" << ir.indexSpaceSize << "  outputs=";
    for (auto& o : ir.outputs) std::cout << o.var << "(" << o.nodeName << ") ";
    std::cout << "\n";

    if (ir.outputs.empty()) { std::cout << "  NO OUTPUTS\n"; return false; }

    SureSimulator<double> sim(ir.system);
    const GraphOutput& out = ir.outputs.front();
    const int M = out.shape[0], N = out.shape[1], K = 3;
    bool ok = true;
    for (int i = 0; i < M; ++i)
        for (int j = 0; j < N; ++j) {
            double got = sim.eval(out.var, IndexPoint({ i, j }));
            bool m = std::abs(got - K) < 1e-9;
            std::cout << "  " << out.var << "[" << i << "][" << j << "] = " << got
                      << (m ? "" : "  MISMATCH (want 3)") << "\n";
            ok &= m;
        }

    // free-schedule legality + memory cardinality on the imported system
    SureSimulator<double> sim2(ir.system);
    ExplicitSchedule s = sim2.computeFreeSchedule();
    std::cout << "  legality: " << checkLegality(ir.system, s) << "\n";
    LivenessReport lr = sim2.analyzeMemory(s);
    long peak = 0; sim2.run(s, &peak);
    std::cout << "  peakLiveValues=" << lr.peakLiveValues << " latency=" << lr.latency
              << " (footprint=" << peak << ")\n";
    ok &= checkLegality(ir.system, s).legal;
    ok &= (peak == lr.peakLiveValues);
    return ok;
}

int main() {
    bool ok = true;

    // 1. import directly from an in-memory graph
    DomainFlowGraph g = buildGraph();
    ImportResult ir = importDomainFlowGraph(g);
    ok &= checkResult(ir, "in-memory");

    // 2. round-trip through a .dfg file to exercise the load() path
    std::string path = (std::filesystem::temp_directory_path() / "dfg_import_sim_test.dfg").string();
    g.save(path);
    ImportResult ir2 = importDfgFile(path);
    ok &= checkResult(ir2, "from .dfg file");
    std::remove(path.c_str());

    std::cout << "\n" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
