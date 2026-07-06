#pragma once
// Import path: lower a DomainFlowGraph (.dfg / MLIR-derived) into a
// RecurrenceSystem the SURE simulator can execute.
//
// Each graph node becomes a recurrence variable "n<id>" over its result-tensor
// index space; graph edges (producer output slot -> consumer input slot) become
// affine dependency taps that read the producer variable directly.  Leaf nodes
// (constants, function arguments) become empty-domain input equations whose
// boundary supplies deterministic data (the .dfg carries types/shapes, not
// values).  Operators are lowered to their recurrence:
//
//   - MATMUL (rank-2):   c(i,j,k) = c(i,j,k-1) + A(i,k)*B(k,j);  n<id>(i,j)=c(i,j,K-1)
//   - elementwise unary/binary (ADD/SUB/MUL/DIV, RELU/SIGMOID/...): n<id> = f(inputs)
//   - shape-preserving passthrough (FUNCTION_RETURN, RESHAPE/TRANSPOSE if identity)
//   - anything else: reported as unsupported and modeled as a zero source so the
//     rest of the graph still elaborates.
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <dfa/dfa.hpp>                 // full library umbrella (correct include ordering, no MLIR)
#include <dfa/sim/sure_simulator.hpp>

namespace sw {
    namespace dfa {
        namespace sim {

            struct ImportReport {
                int nodes = 0;
                int edges = 0;
                std::map<std::string, int> loweredByOp;       // op token -> count
                std::map<std::string, int> unsupportedByOp;   // op token -> count
            };

            struct GraphOutput {
                std::string var;            // recurrence variable holding the output
                std::vector<int> shape;     // its tensor shape
                std::string nodeName;       // source dialect name
            };

            struct ImportResult {
                RecurrenceSystem<double> system;
                std::vector<GraphOutput> outputs;
                ImportReport report;
                long long indexSpaceSize = 0;   // sum of enumerated points (execution cost estimate)
            };

            namespace detail {

                inline std::string opToken(DomainFlowOperator op) {
                    std::ostringstream os; os << op; return os.str();
                }
                inline std::string varName(std::size_t id) { return "n" + std::to_string(id); }

                inline std::vector<int> shapeOf(const std::string& tensorType) {
                    if (tensorType.empty() || tensorType == "n/a") return { 1 };   // absent slot
                    TensorTypeInfo t = parseTensorType(tensorType);
                    if (t.empty()) return { 1 };            // scalar / unparsable -> singleton
                    return t.shape;
                }

                inline long long product(const std::vector<int>& s) {
                    long long p = 1;
                    for (int d : s) p *= (d > 0 ? d : 1);
                    return p;
                }

                inline Domain boxDomain(const std::vector<int>& shape) {
                    std::vector<int> s = shape.empty() ? std::vector<int>{1} : shape;
                    Domain d(static_cast<int>(s.size()));
                    for (int a = 0; a < static_cast<int>(s.size()); ++a)
                        d.axis(a, 0, s[a] > 0 ? s[a] : 1);
                    return d;
                }

                // Identity map, with size-1 producer axes collapsed to index 0 (TOSA broadcast).
                inline AffineDependency broadcastMap(const std::vector<int>& consumerShape,
                                                     const std::vector<int>& producerShape) {
                    int R = static_cast<int>(consumerShape.size());
                    std::vector<std::vector<int>> A(R, std::vector<int>(R, 0));
                    std::vector<int> b(R, 0);
                    for (int r = 0; r < R; ++r) {
                        bool bcast = (r < static_cast<int>(producerShape.size())) &&
                                     producerShape[r] == 1 && consumerShape[r] != 1;
                        if (!bcast) A[r][r] = 1;            // else row stays 0 -> reads index 0
                    }
                    return AffineDependency::map(std::move(A), std::move(b));
                }

                // Rank equality plus per-axis size match (allowing size-1 broadcast axes);
                // gates broadcastMap so incompatible operands fall back to unsupported.
                inline bool shapeBroadcastCompatible(const std::vector<int>& out, const std::vector<int>& in) {
                    if (out.size() != in.size()) return false;
                    for (std::size_t r = 0; r < out.size(); ++r)
                        if (in[r] != 1 && in[r] != out[r]) return false;
                    return true;
                }

                inline bool isLeaf(DomainFlowOperator op) {
                    return op == DomainFlowOperator::CONSTANT ||
                           op == DomainFlowOperator::FUNCTION_ARGUMENT;
                }
                inline bool isSkipped(DomainFlowOperator op) {
                    return op == DomainFlowOperator::FUNCTION ||
                           op == DomainFlowOperator::FUNCTION_RESULT;
                }
                inline bool isPassthrough(DomainFlowOperator op) {
                    return op == DomainFlowOperator::FUNCTION_RETURN ||
                           op == DomainFlowOperator::RESHAPE ||
                           op == DomainFlowOperator::TRANSPOSE;
                }

                inline std::function<double(double)> unaryFn(DomainFlowOperator op) {
                    switch (op) {
                        case DomainFlowOperator::RELU:       return [](double x){ return x > 0 ? x : 0.0; };
                        case DomainFlowOperator::SIGMOID:    return [](double x){ return 1.0 / (1.0 + std::exp(-x)); };
                        case DomainFlowOperator::ABS:        return [](double x){ return std::abs(x); };
                        case DomainFlowOperator::NEGATE:     return [](double x){ return -x; };
                        case DomainFlowOperator::EXP:        return [](double x){ return std::exp(x); };
                        case DomainFlowOperator::ATAN:       return [](double x){ return std::atan(x); };
                        case DomainFlowOperator::RECIPROCAL: return [](double x){ return x != 0 ? 1.0 / x : 0.0; };
                        case DomainFlowOperator::CLAMP:      return [](double x){ return x; };  // attrs ignored
                        case DomainFlowOperator::CAST:       return [](double x){ return x; };
                        default:                             return nullptr;
                    }
                }
                inline std::function<double(double,double)> binaryFn(DomainFlowOperator op) {
                    switch (op) {
                        case DomainFlowOperator::ADD: return [](double a, double b){ return a + b; };
                        case DomainFlowOperator::SUB: return [](double a, double b){ return a - b; };
                        case DomainFlowOperator::MUL: return [](double a, double b){ return a * b; };
                        case DomainFlowOperator::DIV: return [](double a, double b){ return b != 0 ? a / b : 0.0; };
                        default:                      return nullptr;
                    }
                }

                constexpr double kLeafFill = 1.0;    // deterministic value for constants / arguments

            } // namespace detail

            // Lower a loaded DomainFlowGraph into a RecurrenceSystem.
            inline ImportResult importDomainFlowGraph(const DomainFlowGraph& g) {
                using namespace detail;
                ImportResult res;
                RecurrenceSystem<double>& sys = res.system;
                res.report.nodes = static_cast<int>(g.getNrNodes());
                res.report.edges = static_cast<int>(g.getNrEdges());

                // producerOf[(consumerNode, dstSlot)] = producerNode.  Only result slot 0 is
                // modeled (varName covers one value per node); fail fast on multi-output
                // producers rather than silently binding the wrong variable.
                std::map<std::pair<std::size_t, std::size_t>, std::size_t> producerOf;
                for (const auto& [edgeId, edge] : g.edges()) {
                    if (edge.srcSlot != 0)
                        throw std::runtime_error("dfg import: multi-output producer (srcSlot != 0) on node " +
                                                 varName(edgeId.first) + " is not supported");
                    producerOf[{ edgeId.second, edge.dstSlot }] = edgeId.first;
                }

                auto inputVar = [&](std::size_t node, std::size_t slot) -> std::string {
                    auto it = producerOf.find({ node, slot });
                    if (it != producerOf.end()) return varName(it->second);
                    // no incoming edge: synthesize a leaf feeding this operand
                    std::string nm = varName(node) + "_in" + std::to_string(slot);
                    if (!sys.has(nm))
                        sys.add(Equation<double>{ nm, Domain{}, {}, nullptr,
                                                  [](const IndexPoint&) { return kLeafFill; } });
                    return nm;
                };

                auto addLeaf = [&](std::size_t id, double fill) {
                    sys.add(Equation<double>{ varName(id), Domain{}, {}, nullptr,
                                              [fill](const IndexPoint&) { return fill; } });
                };
                auto countLowered = [&](DomainFlowOperator op) { res.report.loweredByOp[opToken(op)]++; };
                auto countUnsupported = [&](DomainFlowOperator op) { res.report.unsupportedByOp[opToken(op)]++; };

                for (const auto& [id, node] : g.nodes()) {
                    DomainFlowOperator op = node.getOperator();
                    if (isSkipped(op)) continue;

                    if (isLeaf(op)) { addLeaf(id, kLeafFill); countLowered(op); continue; }

                    std::vector<int> outShape = shapeOf(node.getResultType(0));
                    res.indexSpaceSize += product(outShape);

                    if (op == DomainFlowOperator::MATMUL) {
                        std::vector<int> a = shapeOf(node.getOperandType(0));   // [M,K]
                        std::vector<int> b = shapeOf(node.getOperandType(1));   // [K,N]
                        if (a.size() != 2 || b.size() != 2 || a[1] != b[0]) { addLeaf(id, 0.0); countUnsupported(op); continue; }
                        int M = a[0], K = a[1], N = b[1];
                        std::string cvar = "c" + std::to_string(id);
                        std::string A = inputVar(id, 0), B = inputVar(id, 1);
                        // c(i,j,k) = c(i,j,k-1) + A(i,k)*B(k,j)
                        sys.add(Equation<double>{
                            cvar, Domain(3).axis(0, 0, M).axis(1, 0, N).axis(2, 0, K),
                            { { cvar, AffineDependency::shift({0, 0, -1}) },
                              { A,    AffineDependency::map({{1,0,0},{0,0,1}}, {0,0}) },   // (i,j,k)->(i,k)
                              { B,    AffineDependency::map({{0,0,1},{0,1,0}}, {0,0}) } }, // (i,j,k)->(k,j)
                            [](const std::vector<double>& t, const IndexPoint&) { return t[0] + t[1] * t[2]; },
                            [](const IndexPoint&) { return 0.0; } });
                        // n<id>(i,j) = c(i,j,K-1)
                        sys.add(Equation<double>{
                            varName(id), Domain(2).axis(0, 0, M).axis(1, 0, N),
                            { { cvar, AffineDependency::map({{1,0},{0,1},{0,0}}, {0, 0, K - 1}) } },
                            [](const std::vector<double>& t, const IndexPoint&) { return t[0]; },
                            [](const IndexPoint&) { return 0.0; } });
                        res.indexSpaceSize += product({ M, N, K });
                        countLowered(op);
                        continue;
                    }

                    if (auto f = binaryFn(op)) {
                        std::vector<int> s0 = shapeOf(node.getOperandType(0));
                        std::vector<int> s1 = shapeOf(node.getOperandType(1));
                        if (!shapeBroadcastCompatible(outShape, s0) || !shapeBroadcastCompatible(outShape, s1)) { addLeaf(id, 0.0); countUnsupported(op); continue; }
                        sys.add(Equation<double>{
                            varName(id), boxDomain(outShape),
                            { { inputVar(id, 0), broadcastMap(outShape, s0) },
                              { inputVar(id, 1), broadcastMap(outShape, s1) } },
                            [f](const std::vector<double>& t, const IndexPoint&) { return f(t[0], t[1]); },
                            [](const IndexPoint&) { return 0.0; } });
                        countLowered(op);
                        continue;
                    }

                    if (auto f = unaryFn(op)) {
                        std::vector<int> s0 = shapeOf(node.getOperandType(0));
                        if (!shapeBroadcastCompatible(outShape, s0)) { addLeaf(id, 0.0); countUnsupported(op); continue; }
                        sys.add(Equation<double>{
                            varName(id), boxDomain(outShape),
                            { { inputVar(id, 0), broadcastMap(outShape, s0) } },
                            [f](const std::vector<double>& t, const IndexPoint&) { return f(t[0]); },
                            [](const IndexPoint&) { return 0.0; } });
                        countLowered(op);
                        continue;
                    }

                    if (isPassthrough(op)) {
                        std::vector<int> s0 = shapeOf(node.getOperandType(0));
                        if (s0 == outShape) {   // shape-preserving copy only
                            sys.add(Equation<double>{
                                varName(id), boxDomain(outShape),
                                { { inputVar(id, 0), broadcastMap(outShape, s0) } },
                                [](const std::vector<double>& t, const IndexPoint&) { return t[0]; },
                                [](const IndexPoint&) { return 0.0; } });
                            countLowered(op);
                        } else {
                            addLeaf(id, 0.0);
                            countUnsupported(op);
                        }
                        continue;
                    }

                    // Unsupported operator: model as a zero source so downstream still elaborates.
                    addLeaf(id, 0.0);
                    countUnsupported(op);
                }

                // Outputs: FUNCTION_RETURN nodes, else declared sinks.
                for (const auto& [id, node] : g.nodes()) {
                    if (node.getOperator() == DomainFlowOperator::FUNCTION_RETURN && sys.has(varName(id)))
                        res.outputs.push_back({ varName(id), shapeOf(node.getResultType(0)), node.getName() });
                }
                if (res.outputs.empty())
                    for (std::size_t snk : g.sink)
                        if (g.has_node(snk) && sys.has(varName(snk)))
                            res.outputs.push_back({ varName(snk), shapeOf(g.node(snk).getResultType(0)), g.node(snk).getName() });

                return res;
            }

            // Convenience: load a .dfg file, then lower it.
            inline ImportResult importDfgFile(const std::string& path) {
                DomainFlowGraph g("imported");
                g.load(path);
                return importDomainFlowGraph(g);
            }

            inline std::ostream& operator<<(std::ostream& os, const ImportReport& r) {
                os << "nodes=" << r.nodes << " edges=" << r.edges << "\n";
                os << "  lowered:";
                for (const auto& [op, n] : r.loweredByOp) os << " " << op << "=" << n;
                os << "\n";
                if (!r.unsupportedByOp.empty()) {
                    os << "  unsupported:";
                    for (const auto& [op, n] : r.unsupportedByOp) os << " " << op << "=" << n;
                    os << "\n";
                }
                return os;
            }

        }
    }
}
