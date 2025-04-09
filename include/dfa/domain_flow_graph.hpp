#pragma once
#include <iostream>
#include <iomanip>

// extendable graph data structure
#include <graph/graph.hpp>
#include <dfa/arithmetic_complexity.hpp>

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
			void clear() { graph.clear(); }
			void setName(const std::string& name) { this->name = name; }
			void addNode(const std::string& name) {
				DomainFlowNode node(name);
				graph.add_node(node);
			}
			void addNode(DomainFlowOperator opType, const std::string& name) {
				DomainFlowNode node(opType, name);
				graph.add_node(node);
			}
			void addNode(const DomainFlowNode& node) {
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

			ArithmeticMetrics arithmeticComplexity() const {
				ArithmeticMetrics metrics;
				for (auto& node : graph.nodes()) {
					auto work = node.second.getArithmeticComplexity();
					for (auto& stats : work) {
						std::string opType = std::get<0>(stats);
						std::string numType = std::get<1>(stats);
						std::uint64_t opsCount = std::get<2>(stats);
						//std::cout << "Operator: " << opType << ", Type: " << numType << ", Count: " << opsCount << std::endl;
						metrics.recordOperation(opType, numType, opsCount);
					}
				}
				return metrics;
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

		// Generate the arithmetic complexity table
		inline void reportArithmeticComplexity(const DomainFlowGraph& g) {
			std::cout << "Arithmetic complexity:" << '\n';
			// walk the graph and accumulate all arithmetic operations
			auto arithOps = g.arithmeticComplexity();
			// gather the total
			uint64_t total = 0;
			for (const auto& opType : arithOps.getOperationTypes()) {
				for (const auto& [numType, count] : arithOps.opMetrics.at(opType)) {
					total += count;
				}
			}
			const int OPERATOR_WIDTH = 25;
			const int COL_WIDTH = 15;
			// Print the header
			std::cout << std::setw(OPERATOR_WIDTH) << "Arithmetic Op" << std::setw(COL_WIDTH) << "count" << std::setw(COL_WIDTH) << "Percentage" << std::endl;
			for (auto& opType : arithOps.getOperationTypes()) {
				uint64_t opTypeTotal = arithOps.getOperationTotal(opType);
				std::cout << std::setw(OPERATOR_WIDTH) << std::left << opType
					<< std::setw(COL_WIDTH) << std::right << opTypeTotal
					<< std::setw(COL_WIDTH) << (opTypeTotal * 100.0)/total << '\n';
				
				// sort the numerical types
				std::vector<std::string> orderedNumTypes = { "i8", "i16", "i32", "f8", "f16", "f32", "f64" };
				std::map<std::string, uint64_t> sortedNumTypeMetrics;
				for (const auto& [numType, count] : arithOps.opMetrics[opType]) {
					sortedNumTypeMetrics[numType] = count;
				}
				for (const auto& numType : orderedNumTypes) {
					const auto count = sortedNumTypeMetrics[numType];
					std::cout << std::setw(OPERATOR_WIDTH) << std::left << (std::string("     ") + numType)
						<< std::setw(COL_WIDTH) << std::right << count 
						<< std::setw(COL_WIDTH) << (count * 100.0)/total << '\n';
				}
			}
		}

		// Generate the numerical complexity table
		inline void reportNumericalComplexity(const DomainFlowGraph& g) {
			std::cout << "Numerical complexity:" << '\n';
			// walk the graph and accumulate all arithmetic operations
			auto arithOps = g.arithmeticComplexity();
			// gather the total
			uint64_t total = 0;
			for (const auto& opType : arithOps.getOperationTypes()) {
				for (const auto& [numType, count] : arithOps.opMetrics.at(opType)) {
					total += count;
				}
			}
			const int OPERATOR_WIDTH = 25;
			const int COL_WIDTH = 15;
			// Normalized by numerical type
			// Print the header
			std::cout << std::setw(OPERATOR_WIDTH) << "Arithmetic Op" << std::setw(COL_WIDTH) << "count" << std::setw(COL_WIDTH) << "Percentage" << std::endl;
			// sort the numerical types
			std::vector<std::string> orderedNumTypes = { "i8", "i16", "i32", "f8", "f16", "f32", "f64" };
			for (auto& numType : orderedNumTypes) {
				uint64_t numTypeTotal = arithOps.getNumericalTypeTotal(numType);
				std::cout << std::setw(OPERATOR_WIDTH) << std::left << numType
					<< std::setw(COL_WIDTH) << std::right << numTypeTotal
					<< std::setw(COL_WIDTH) << (numTypeTotal * 100.0) / total << '\n';

				for (const auto& [opType, typeMap] : arithOps.opMetrics) {
					auto it = typeMap.find(numType);
					if (it != typeMap.end()) {
						auto count = it->second;
						std::cout << std::setw(OPERATOR_WIDTH) << std::left << (std::string("     ") + opType)
							<< std::setw(COL_WIDTH) << std::right << count
							<< std::setw(COL_WIDTH) << (count * 100.0) / total << '\n';
					}
					else {
						std::cout << std::setw(OPERATOR_WIDTH) << std::left << (std::string("     no ") + numType + std::string(" ops")) << '\n';
					}
				}
			}
		}
    }
}

