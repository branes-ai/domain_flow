#pragma once
#include <iostream>
#include <iomanip>

// extendable graph data structure
#include <graph/graphlib.hpp>
#include <dfa/arithmetic_complexity.hpp>

namespace sw {
    namespace dfa {

		using domain_flow_graph = sw::graph::directed_graph<DomainFlowNode, DomainFlowEdge>;

        // Definition of the Domain Flow graph
		struct DomainFlowGraph {
			// lifting the base graph types into the DomainFlowGraph
			using nodeId_to_node_t = typename sw::graph::graph<DomainFlowNode, DomainFlowEdge, sw::graph::DIRECTED_GRAPH>::nodeId_to_node_t;
			using edgeId_to_edge_t = typename sw::graph::graph<DomainFlowNode, DomainFlowEdge, sw::graph::DIRECTED_GRAPH>::edgeId_to_edge_t;
			using node_t           = typename sw::graph::graph<DomainFlowNode, DomainFlowEdge, sw::graph::DIRECTED_GRAPH>::node_t;
			using edge_t           = typename sw::graph::graph<DomainFlowNode, DomainFlowEdge, sw::graph::DIRECTED_GRAPH>::edge_t;
			using ConstraintCoefficientType = int;

			std::string name;
			domain_flow_graph graph{};
			// sources and sinks: domains flow from sources to sinks
			std::vector<sw::graph::nodeId_t> source;
			std::vector<sw::graph::nodeId_t> sink;

			DomainFlowGraph(std::string name) : name{ name } {}
			DomainFlowGraph(std::string name, sw::graph::directed_graph<DomainFlowNode, DomainFlowEdge> graph,
				std::vector<sw::graph::nodeId_t> source, std::vector<sw::graph::nodeId_t> sink) :
				name{ name }, graph{ graph }, source{ source }, sink{ sink } {
			}
			~DomainFlowGraph() {}

			//////////////////////////////////////////////////////////////////////////////////////////////////////
			// modifiers
			void clear() { graph.clear(); source.clear(); sink.clear(); name.clear(); }
			void setName(const std::string& name) { this->name = name; }
			void setSchedule(const Schedule<int>& tau) { /*graph.setSchedule(tau);*/ }

			void addNode(const std::string& name) {
				DomainFlowNode node(name);
				graph.add_node(node);
			}
			sw::graph::nodeId_t addNode(DomainFlowOperator opType, const std::string& name) {
				DomainFlowNode node(opType, name);
				return graph.add_node(node);
			}
			sw::graph::nodeId_t addNode(const DomainFlowNode& node) {
				return graph.add_node(node);
			}

			void addEdge(sw::graph::nodeId_t src, std::size_t outputSlot, sw::graph::nodeId_t dest, std::size_t inputSlot, const DomainFlowEdge& edge) {
				DomainFlowEdge edgeCopy = edge;
				edgeCopy.srcSlot = outputSlot;
				edgeCopy.dstSlot = inputSlot;
				graph.add_edge(src, dest, edgeCopy);
			}

			void addSource(sw::graph::nodeId_t src) {
				source.push_back(src);
			}
			void addSink(sw::graph::nodeId_t sink) {
				this->sink.push_back(sink);
			}

			// sort the nodes in the graph
			// Assign depth values to nodes based on their maximum distance from inputs
			void assignNodeDepths() noexcept {
				constexpr bool bTrace = false; // Set to true for detailed tracing
				auto nodeDepths = calculateNodeDepths(graph);
				// Store depth values in the operator nodes
				for (size_t i = 0; i < graph.nrNodes(); ++i) {
					// Access node data and set depth
					DomainFlowNode& op = graph.node(i);
					op.setDepth(static_cast<int>(nodeDepths[i]));
				}
			}

			// distributed constants throughout the graph so they are close to their first use
			void distributeConstants() noexcept { graph.distributeConstants(); }

			// extract a subgraph of the DFG based on starting and ending depth
			DomainFlowGraph subgraph(int startDepth, int endDepth) const {
				DomainFlowGraph subgraph(name + "_subgraph");
				subgraph.graph.clear();
				subgraph.graph = graph.subgraph(startDepth, endDepth);
				for (const auto& src : source) {
					if (subgraph.graph.has_node(src)) {
						subgraph.source.push_back(src);
					}
				}
				for (const auto& snk : sink) {
					if (subgraph.graph.has_node(snk)) {
						subgraph.sink.push_back(snk);
					}
				}
				return subgraph;
			}

			void instantiateDomains() noexcept { 
				// walk the graph, and generate the DoC for each operator
				for (const auto& nodeId : graph.nodes()) {
					if (graph.in_degree(nodeId.first) > 0) { // filter out inputs
						DomainFlowNode& node = graph.node(nodeId.first);
						node.instantiateDomain();
					}
				}
			}

			void instantiateIndexSpaces() noexcept { 
				// walk the graph, and generate the DoC and IndesSpace for each operator
				for (const auto& nodeId : graph.nodes()) {
					if (graph.in_degree(nodeId.first) > 0) { // filter out inputs
						DomainFlowNode& node = graph.node(nodeId.first);
						node.instantiateDomain();
						node.instantiateIndexSpace();
					}
				}
			}

			void applyLinearSchedule(const ScheduleVector<int>& tau) noexcept { 
				// walk the graph, and apply the linear schedule to each operator
				for (const auto& nodeId : graph.nodes()) {
					if (graph.in_degree(nodeId.first) > 0) { // filter out inputs
						DomainFlowNode& node = graph.node(nodeId.first);
						node.applyLinearSchedule(tau);
					}
				}
			}

			//////////////////////////////////////////////////////////////////////////////////////////////////////
			// selectors
			std::string getName() const noexcept { return name; }
			std::size_t getNrNodes() const noexcept { return graph.nrNodes(); }
			std::size_t getNrEdges() const noexcept { return graph.nrEdges(); }
			bool has_node(sw::graph::nodeId_t node_id) const noexcept { return graph.has_node(node_id); }
			bool has_edge(sw::graph::nodeId_t node_id_lhs, sw::graph::nodeId_t node_id_rhs) const noexcept { return graph.has_edge(node_id_lhs, node_id_rhs); }
			const nodeId_to_node_t& nodes() const noexcept { return graph.nodes(); }
			const edgeId_to_edge_t& edges() const noexcept { return graph.edges(); }
			const node_t& node(sw::graph::nodeId_t node_id) const { return graph.node(node_id); }
			const edge_t& edge(sw::graph::nodeId_t lhs, sw::graph::nodeId_t rhs) const { return graph.edge(lhs, rhs); }
			const edge_t& edge(const sw::graph::edgeId_t& edge_id) const { return graph.edge(edge_id); }

			// get the convex hull of a node
			ConvexHull<ConstraintCoefficientType> getConvexHull(sw::graph::nodeId_t nodeId) const noexcept {
				ConvexHull<ConstraintCoefficientType> hull;
				for (const auto& node : graph.nodes()) {
					if (node.first == nodeId) {
						hull = node.second.getConvexHull();
					}
				}
				// return an empty PointSet if the node is not found
				return hull;
			}
			PointSet<ConstraintCoefficientType> getConvexHullPointSet(sw::graph::nodeId_t nodeId) const noexcept {
				PointSet<ConstraintCoefficientType> pointSet;
				for (const auto& node : graph.nodes()) {
					if (node.first == nodeId) {
						pointSet = node.second.getConvexHullPointSet();
					}
				}
				// return an empty PointSet if the node is not found
				return pointSet;
			}
			// get the tensor confluences for a node
			ConfluenceSet<ConstraintCoefficientType> getConfluences(sw::graph::nodeId_t nodeId) const noexcept {
				ConfluenceSet<ConstraintCoefficientType> confluenceSet;
				for (const auto& node : graph.nodes()) {
					if (node.first == nodeId) {
						confluenceSet = node.second.getConfluences();
					}
				}
				return confluenceSet;
			}
			// get the constraint set of a node
			ConstraintSet<ConstraintCoefficientType> getConstraints(sw::graph::nodeId_t nodeId) const noexcept {
				for (const auto& node : graph.nodes()) {
					if (node.first == nodeId) {
						return node.second.getConstraints();
					}
				}
				// return an empty ConstraintSet if the node is not found
				return ConstraintSet<ConstraintCoefficientType>();
			}

			// Generate the schedule for the graph
			void generateSchedule(ScheduleVector<ConstraintCoefficientType>& tau) const noexcept {
				tau.clear();
				tau.assign({ 1, 1, 1 });
				// TODO: analyze the index spaces that make up the pipeline and generate a valid schedule
				std::cerr << "DomainFlowGraph::generateSchedule: not implemented yet" << std::endl;
			}

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

			// Save the graph to a text file
			void save(const std::string& filename) const {
				std::ofstream ofs(filename);
				if (!ofs) {
					throw std::runtime_error("Failed to open file for writing: " + filename);
				}
				// Save the graph to the file
				save(ofs);

				if (!ofs.good()) {
					throw std::runtime_error("Error occurred while writing to file: " + filename);
				}

				ofs.close();
			}
			// Save the graph to an output stream
			std::ostream& save(std::ostream& ostr) const {
				ostr << "Domain Flow Graph: " << name << "\n";
				graph.save(ostr);
				ostr << "SOURCE: ";
				bool first = true;
				for (const auto& src : source) {
					if (!first) {
						ostr << ", ";
					}
					ostr << src;
					first = false;
				}
				ostr << "\nSINK: ";
				first = true;
				for (const auto& snk : sink) {
					if (!first) {
						ostr << ", ";
					}
					ostr << snk;
				}
				return ostr;
			}
			// Load the graph from a text file
			void load(const std::string& filename) {
				std::ifstream ifs(filename);
				if (!ifs) {
					throw std::runtime_error("Failed to open file for reading: " + filename);
				}

				load(ifs);
				
				if (ifs.fail()) {
					throw std::runtime_error("Error occurred while reading from file: " + filename);
				}

				ifs.close();
			}
			// Load the graph from an input stream
			std::istream& load(std::istream& istr) {
				clear();
				std::string line;
				if (!std::getline(istr, line)) {
					istr.setstate(std::ios::failbit);
					return istr;
				}
				// search for :
				size_t colonPos = line.find(':');
				if (colonPos == std::string::npos) {
					istr.setstate(std::ios::failbit);
					return istr;
				}
				// Extract substring after colon and trim whitespace
				std::string dfgName = line.substr(colonPos + 1);
				while (!dfgName.empty() && std::isspace(dfgName.front())) {
					dfgName.erase(0, 1);
				}
				this->name = dfgName;
				//std::cout << "Stream state after graph name: " << istr.good() << " " << istr.fail() << " " << istr.eof() << std::endl;

				///////////////////////////////////////////////////////
				// Read the graph from the input stream
				graph.load(istr);
				//std::cout << "Stream state after graph.load: " << istr.good() << " " << istr.fail() << " " << istr.eof() << std::endl;
				///////////////////////////////////////////////////////
				if (!std::getline(istr, line)) {
					istr.setstate(std::ios::failbit);
					return istr;
				}
				colonPos = line.find(':');
				if (colonPos == std::string::npos) {
					istr.setstate(std::ios::failbit);
					return istr;
				}
				std::string sources = line.substr(colonPos + 1);
				// Trim whitespace from sources
				while (!sources.empty() && std::isspace(sources.front())) {
					sources.erase(0, 1);
				}
				while (!sources.empty() && std::isspace(sources.back())) {
					sources.pop_back();
				}

				// Parse sources
				if (!sources.empty()) {
					std::stringstream ss(sources);
					std::string temp;
					while (std::getline(ss, temp, ',')) {
						while (!temp.empty() && std::isspace(temp.front())) {
							temp.erase(0, 1);
						}
						while (!temp.empty() && std::isspace(temp.back())) {
							temp.pop_back();
						}
						if (!temp.empty()) {
							try {
								source.push_back(std::stoi(temp));
							}
							catch (const std::exception&) {
								istr.setstate(std::ios::failbit);
								return istr;
							}
						}
					}
					// Handle single number case (no commas)
					if (source.empty()) {
						try {
							source.push_back(std::stoi(sources));
						}
						catch (const std::exception&) {
							istr.setstate(std::ios::failbit);
							return istr;
						}
					}
				}
				//std::cout << "Stream state after SOURCES: " << istr.good() << " " << istr.fail() << " " << istr.eof() << std::endl;

				// Read sinks
				if (!std::getline(istr, line)) {
					istr.setstate(std::ios::failbit);
					return istr;
				}
				colonPos = line.find(':');
				if (colonPos == std::string::npos) {
					istr.setstate(std::ios::failbit);
					return istr;
				}
				std::string sinks = line.substr(colonPos + 1);
				// Trim whitespace from sinks
				while (!sinks.empty() && std::isspace(sinks.front())) {
					sinks.erase(0, 1);
				}
				while (!sinks.empty() && std::isspace(sinks.back())) {
					sinks.pop_back();
				}

				// Parse sinks
				if (!sinks.empty()) {
					std::stringstream ss(sinks);
					std::string temp;
					while (std::getline(ss, temp, ',')) {
						while (!temp.empty() && std::isspace(temp.front())) {
							temp.erase(0, 1);
						}
						while (!temp.empty() && std::isspace(temp.back())) {
							temp.pop_back();
						}
						if (!temp.empty()) {
							try {
								sink.push_back(std::stoi(temp));
							}
							catch (const std::exception&) {
								istr.setstate(std::ios::failbit);
								return istr;
							}
						}
					}
					// Handle single number case (no commas)
					if (sink.empty()) {
						try {
							sink.push_back(std::stoi(sinks));
						}
						catch (const std::exception&) {
							istr.setstate(std::ios::failbit);
							return istr;
						}
					}
				}
				//std::cout << "Stream state after SINKS: " << istr.good() << " " << istr.fail() << " " << istr.eof() << std::endl;
				// the expected state is that at this point, istr.eof() is true, and istr.fail() is false. istr.good() is useless at this point
				// the receiver should check for istr.fail() for any caught issues and adapt accordingly.
				return istr;
			}
		};

		inline bool operator==(const DomainFlowGraph& lhs, const DomainFlowGraph& rhs) {
			bool bEqual = true;

			// compare the nodes
			if (lhs.graph.nrNodes() != rhs.graph.nrNodes()) {
				bEqual = false;
			}
			else {
				for (const auto& [nodeId, node] : lhs.graph.nodes()) {
					if (rhs.graph.has_node(nodeId)) {
						const auto& rhsNode = rhs.graph.node(nodeId);
						if (node != rhsNode) {
							bEqual = false;
							break;
						}
					}
					else {
						bEqual = false;
						break;
					}
				}
			}

			// compare the edges
			if (lhs.graph.nrEdges() != rhs.graph.nrEdges()) {
				bEqual = false;
			}
			else {
				for (const auto& [edgeId, edge] : lhs.graph.edges()) {
					if (rhs.graph.has_edge(edgeId.first, edgeId.second)) {
						const auto& rhsEdge = rhs.graph.edge(edgeId);
						if (edge != rhsEdge) {
							bEqual = false;
							break;
						}
					}
					else {
						bEqual = false;
						break;
					}
				}
			}

			return bEqual;
		}
		inline bool operator!=(const DomainFlowGraph& lhs, const DomainFlowGraph& rhs) {
			return !(lhs == rhs);
		}

		inline std::ostream& operator<<(std::ostream& ostr, const DomainFlowGraph& g) {
			return g.save(ostr);
		}

		inline std::istream& operator>>(std::istream& istr, DomainFlowGraph& g) {
			return g.load(istr);
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

