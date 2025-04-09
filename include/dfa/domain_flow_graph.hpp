#pragma once
#include <iostream>
#include <iomanip>

// extendable graph data structure
#include <graph/graph.hpp>

namespace sw {
    namespace dfa {

		using domain_flow_graph = sw::graph::directed_graph<DomainFlowNode, DomainFlowEdge>;

        // Definition of the Domain Flow graph
		struct DomainFlowGraph {
			std::string name;
			domain_flow_graph graph{};
			std::vector<sw::graph::nodeId_t> source;
			std::vector<sw::graph::nodeId_t> sink;

			DomainFlowGraph(std::string name) : name{ name } {}
			DomainFlowGraph(std::string name, sw::graph::directed_graph<DomainFlowNode, DomainFlowEdge> graph,
				std::vector<sw::graph::nodeId_t> source, std::vector<sw::graph::nodeId_t> sink) :
				name{ name }, graph{ graph }, source{ source }, sink{ sink } {
			}
			~DomainFlowGraph() {}

			// Modifiers
			void setName(const std::string& name) { this->name = name; }
			void addNode(const std::string& name) {
				DomainFlowNode node(name);
				graph.add_node(node);
			}
			void addNode(DomainFlowOperator opType, const std::string& name) {
				DomainFlowNode node(opType, name);
				graph.add_node(node);
			}

			// Selectors
			std::string getName() const noexcept { return name; }
			std::size_t getNrNodes() const noexcept { return graph.nrNodes(); }
			std::size_t getNrEdges() const noexcept { return graph.nrEdges(); }

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

			std::ostream& save(std::ostream& ostr) const {
				ostr << "Domain Flow Graph: " << name << "\n";
				graph.save(ostr);
				return ostr;
			}

		};

		std::ostream& operator<<(std::ostream& ostr, const DomainFlowGraph& g) {
			ostr << "Domain Flow Graph: " << g.name << "\n";
				g.graph.save(ostr);
			return ostr;
		}

		std::istream& operator>>(std::istream& istr, DomainFlowGraph& g) {
			std::string line;
			if (!std::getline(istr, line)) {
				istr.setstate(std::ios::failbit);
				return istr;
			}
			g.name = line;
			// Read the graph from the input stream
			g.graph.load(istr);
			return istr;
		}

		// Generate the operator statistics table
		inline void reportOperatorStats(const DomainFlowGraph& g) {
			// Generate operator statistics
			std::cout << "Operator statistics:" << std::endl;
			auto opCount = g.operatorStats();
			const int OPERATOR_WIDTH = 25;
			const int COL_WIDTH = 15;
			// Print the header
			std::cout << std::setw(OPERATOR_WIDTH) << "Operator" << std::setw(COL_WIDTH) << "count" << std::setw(COL_WIDTH) << "Percentage" << std::endl;
			// Print the operator statistics
			for (const auto& [op, cnt] : opCount) {
				std::cout << std::setw(OPERATOR_WIDTH) << op << std::setw(COL_WIDTH) << cnt
					<< std::setprecision(2) << std::fixed
					<< std::setw(COL_WIDTH - 1) << (cnt * 100.0 / g.graph.nrNodes()) << "%" << std::endl;
			}
		}
    }
}

