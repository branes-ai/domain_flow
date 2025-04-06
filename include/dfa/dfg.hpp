#pragma once

// extendable graph data structure
#include <graph/graph.hpp>

namespace sw {
    namespace dfa {

        // the Domain Flow Graph node type
        struct DomainFlowOperator {
            std::string operatorName;
            int depth;   // 0 is a source
            std::vector<std::string> resultValue;    // string version of mlir::Value: typically too verbose
            std::vector<std::string> resultType;     // string version of mlir::Type
			std::vector<std::string> inputType;      // string version of mlir::Type

            // Constructor to initialize the node with just a string of the operator
            DomainFlowOperator(std::string name) : operatorName{ name }, depth{ 0 } {}
            void setDepth(int d) { depth = d; }
            void setOperator(std::string name) { this->operatorName = name; }

            void addResult(const std::string& valueStr, const std::string& typeStr) {
                resultValue.push_back(valueStr);
                resultType.push_back(typeStr);
            }
			void addInput(const std::string& typeStr) {
				inputType.push_back(typeStr);
			}
            std::string getName() const noexcept { return operatorName; }
            int getDepth() const noexcept { return depth; }
            std::string getResultValue(std::size_t idx) const noexcept { return resultValue[idx]; }
            std::string getResultType(std::size_t idx) const noexcept { return resultType[idx]; }
        };

        std::ostream& operator<<(std::ostream& ostr, const DomainFlowOperator& op) {
            ostr << op.operatorName << " at depth " << op.depth;
            for (std::size_t idx = 0; idx < op.resultValue.size(); ++idx) {
                ostr << " -> " << op.resultValue[idx] << " of type " << op.resultType[idx];
            }
            return ostr;
        }

		// the Domain Flow Graph node type capturing the TensorProduct operation
		struct TensorProduct : public DomainFlowOperator {
			TensorProduct(std::string name) : DomainFlowOperator(name) {}
			~TensorProduct() {}
		};
		// the Domain Flow Graph node type capturing the Convolution operation
        struct Convolution : public DomainFlowOperator {
            Convolution(std::string name) : DomainFlowOperator(name) {}
            ~Convolution() {}
        };

        // the Domain Flow Graph edge type
        struct DomainFlow : public sw::graph::weighted_edge<int> { // Weighted by the data flow on this link
            int flow;
            bool stationair;  // does the flow go through a memory or not

            int weight() const noexcept override {
                return flow;
            }
            DomainFlow(int flow, bool stationair = true) : flow{ flow }, stationair{ stationair } {}
            ~DomainFlow() {}
        };


        // Definition of the Domain Flow graph
		struct DomainFlowGraph {
            std::string name;
			sw::graph::directed_graph<DomainFlowOperator, DomainFlow> graph{};
			std::vector<sw::graph::nodeId_t> source;
			std::vector<sw::graph::nodeId_t> sink;

			DomainFlowGraph(std::string name) : name{ name } {}
			DomainFlowGraph(std::string name, sw::graph::directed_graph<DomainFlowOperator, DomainFlow> graph,
				std::vector<sw::graph::nodeId_t> source, std::vector<sw::graph::nodeId_t> sink) :
				name{ name }, graph{ graph }, source{ source }, sink{ sink } {
			}
			~DomainFlowGraph() {}

            std::map<std::string, int> operatorStats() const {
				std::map<std::string, int> opCount;
                for (auto& node : graph.nodes()) {
                    auto op = node.second.getName();
                    if (opCount.find(op) == opCount.end()) {
                        opCount[op] = 1;
                    }
                    else {
                        opCount[op]++;
                    }
                }
                return opCount;
            }
		};
		std::ostream& operator<<(std::ostream& ostr, const DomainFlowGraph& g) {
			ostr << "Domain Flow Graph: " << g.name << "\n";
            // Print the nodes and their properties
            for (auto& node : g.graph.nodes()) {
                std::cout << "Node ID: " << node.first << ": " << node.second << " In degree: " << g.graph.in_degree(node.first) << " Out degree: " << g.graph.out_degree(node.first) << '\n';
            }
			return ostr;
		}


    }
}

