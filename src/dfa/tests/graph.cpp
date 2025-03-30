#include <iostream>
#include <iomanip>
#include <format>
#include <filesystem>
#include <dfa/dfa.hpp>
#include <dfa/algorithm/shortest_path/dijkstra.hpp>

namespace sw {
    namespace dfa {

        struct Operator {
            std::string name;
        };

        struct Flow : public weighted_edge<int> { // Weighted by the data flow on this link
            int flow;
            bool stationair;  // does the flow go through a memory or not

            [[nodiscard]] int weight() const noexcept override {
                return flow;
            }
            Flow(int flow, bool stationair) : flow{ flow }, stationair{ stationair } {}
            ~Flow() {}
        };

        struct Time : public weighted_edge<float> { // Weighted by time the schedule takes to move the data
            std::string airline;
        };

        struct Space : public weighted_edge<float> { // Weighted by spatial 'width' the data occupies
            std::string airline;
        };
    
        struct DeepLearningGraph {
            directed_graph<Operator, Flow> graph{};
            vertex_id_t source{};
            vertex_id_t sink{};
        };

        DeepLearningGraph create_dfa_graph() {
            // Create an directed graph to represent the Deep Learning domain flow graph
            directed_graph<Operator, Flow> dl_graph;

            // Add operators (vertices)
            auto InputTensor = dl_graph.add_vertex("Input Image");
            auto Linear = dl_graph.add_vertex("Linear");
            auto Bias = dl_graph.add_vertex("Bias");
            auto ReLU = dl_graph.add_vertex("ReLU");
            auto OutputTensor = dl_graph.add_vertex("Output Classification");

            // Add data flow connection
            dl_graph.add_edge(InputTensor, Linear, Flow{ 1024, true });
            dl_graph.add_edge(Linear, Bias, Flow{ 32, false });
            dl_graph.add_edge(Bias, ReLU, Flow{ 32, false });
            dl_graph.add_edge(ReLU, OutputTensor, Flow{ 32, false });

            return { dl_graph, InputTensor, OutputTensor };
        }

        inline void print_shortest_path(
            const directed_graph<Operator, Flow>& graph,
            const std::optional<algorithm::shortest_path::graph_path<decltype(weight(std::declval<Flow>()))>>& path,
            const std::string& filepath) 
        {
            auto shortest_path{ path.value() };
            std::unordered_set<edge_id_t, edge_id_hash> edges_on_shortest_path{};

            vertex_id_t prev{ shortest_path.vertices.front() };
            shortest_path.vertices.pop_front();
            for (const auto current : shortest_path.vertices) {
                edges_on_shortest_path.insert(std::make_pair(prev, current));
                prev = current;
            }

            const auto vertex_writer{ [](vertex_id_t vertex_id, Operator vertex) -> std::string {
              const auto style{"filled"};
              return std::format(
                  "label=\"{}\", style={}, color=black, fontcolor=black, shape=rectangle,"
                  "fillcolor=mediumspringgreen",
                  vertex.name, style);
            } };

            const auto edge_writer{ [&edges_on_shortest_path](
                                       const edge_id_t& edge_id,
                                       const auto& edge) -> std::string {
              const auto style{"solid"};
              if (edges_on_shortest_path.contains({edge_id.first, edge_id.second}) ||
                  edges_on_shortest_path.contains({edge_id.second, edge_id.first})) {
                return std::format("label=\"{}\", style={}, color=red, fontcolor=black",
                                   edge.kilometers, style);
              }
              return std::format("label=\"{}\", style={}, color=black, fontcolor=black",
                                 edge.kilometers, style);
            } };

            const std::filesystem::path output{ filepath };
            //io::to_dot(graph, output, vertex_writer, edge_writer);
        }
    }
}


int main() {
    using namespace sw::dfa;
    const auto [graph, source, sink] {create_dfa_graph()};

    const auto weighted_shortest_path{ algorithm::shortest_path::dijkstra(graph, source, sink) };
    print_shortest_path(graph, weighted_shortest_path, "oneLayerMLP.dot");

    return EXIT_SUCCESS;
}
