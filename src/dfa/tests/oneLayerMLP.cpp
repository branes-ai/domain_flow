#include <iostream>
#include <iomanip>
#include <format>
#include <filesystem>
#include <dfa/dfa.hpp>


namespace sw {
    namespace dfa {
        namespace graph {

            enum class TensorOp { ADD, SUB, MUL, MATVEC, MATMUL };

            struct TensorOperator {
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

            struct DeepLearningGraph {
                directed_graph<TensorOperator, Flow> graph{};
                nodeId_t input{};
                nodeId_t output{};
            };

            DeepLearningGraph create_dfa_graph() {
                // Create an directed graph to represent the Deep Learning domain flow graph
                directed_graph<TensorOperator, Flow> dl_graph;

                // Add operators (nodes)
                auto InputTensor = dl_graph.add_node("Input Image");
                auto Linear = dl_graph.add_node("Linear");
                auto Bias = dl_graph.add_node("Bias");
                auto ReLU = dl_graph.add_node("ReLU");
                auto OutputTensor = dl_graph.add_node("Output Classification");

                // Add data flow connection
                dl_graph.add_edge(InputTensor, Linear, Flow{ 1024, true });
                dl_graph.add_edge(Linear, Bias, Flow{ 32, false });
                dl_graph.add_edge(Bias, ReLU, Flow{ 32, false });
                dl_graph.add_edge(ReLU, OutputTensor, Flow{ 32, false });

                return { dl_graph, InputTensor, OutputTensor };
            }

            template <typename V, typename E, bool D, typename VertexWriter, typename EdgeWriter>
                requires std::is_invocable_r_v<std::string, const VertexWriter&, nodeId_t, const V&>&&
            std::is_invocable_r_v<std::string, const EdgeWriter&, const edgeId_t&, const typename graph<V, E, D>::edge_t&>
                void print_graph(std::ostream& ostr, const graph<V, E, D>& graph,
                    const VertexWriter& vertex_writer,
                    const EdgeWriter& edge_writer) {

                if constexpr (D) {
                    ostr << "Directed Graph:\n";
                }
                else {
                    ostr << "Undirected Graph:\n";
                }

                for (const auto& [vertex_id, vertex] : graph.nodes()) {
                    ostr << "  " << std::to_string(vertex_id) << " : " << vertex_writer(vertex_id, vertex) << '\n';
                }

                //const auto edge_specifier{ detail::graph_type_to_edge_specifier(T) };
                const auto edge_specifier = "direct";
                for (const auto& [edge_id, edge] : graph.edges()) {
                    const auto [input_id, target_id] {edge_id};
                    ostr << "\t" << std::to_string(input_id) << " " << edge_specifier << " " << std::to_string(target_id) << " [" << edge_writer(edge_id, edge) << "]\n";
                }

                ostr << '\n';
            }

            inline void print_shortest_path(std::ostream& ostr, const directed_graph<TensorOperator, Flow>& g,
                const std::optional<algorithm::shortest_path::graph_path<decltype(weight(std::declval<Flow>()))>>& path)
            {
                auto shortest_path{ path.value() };
                std::unordered_set<edgeId_t, edge_id_hash> edges_on_shortest_path{};

                nodeId_t prev{ shortest_path.nodes.front() };
                shortest_path.nodes.pop_front();
                for (const auto current : shortest_path.nodes) {
                    edges_on_shortest_path.insert(std::make_pair(prev, current));
                    prev = current;
                }

                const auto vertex_writer{ [](nodeId_t vertex_id, TensorOperator vertex) -> std::string {
                    return std::format("node={}", vertex.name);
                } };

                //const auto edge_writer{ [](edgeId_t edge_id, Flow edge) -> std::string {
                //    return std::format("edge_id={},{} value={}", edge_id.first, edge_id.second, edge.flow);
                //} };
                const auto edge_writer{ [&edges_on_shortest_path](
                             const edgeId_t& edge_id,
                             const auto& edge) -> std::string
                {
                    if (edges_on_shortest_path.contains({edge_id.first, edge_id.second}) ||
                        edges_on_shortest_path.contains({edge_id.second, edge_id.first})) {
                    return std::format("edge_id=[{},{}] value={}", edge_id.first, edge_id.second, edge.flow);
                    }
                    return std::format("edge_id=[{},{}] value={}", edge_id.first, edge_id.second, edge.flow);
                } };

                print_graph(ostr, g, vertex_writer, edge_writer);
            }

        }
    }
}


int main() {
    using namespace sw::dfa::graph;
    const auto [graph, input, output] {create_dfa_graph()};

    const auto weighted_shortest_path{ algorithm::shortest_path::dijkstra(graph, input, output) };
    print_shortest_path(std::cout, graph, weighted_shortest_path);

    //const std::filesystem::path& path{ "oneLayerMLP.dot" }
    //std::ofstream dot_file{ path };

    return EXIT_SUCCESS;
}
