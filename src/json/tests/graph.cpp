#include <iostream>
#include <iomanip>
#include <format>
#include <filesystem>
#include <graph/graph.hpp>

namespace sw {
    namespace graph {

        struct Operator {
            std::string name;
        };

        struct Flow : public weighted_edge<int> { // Weighted by the data flow on this link
            int flow;
            bool stationair;  // does the flow go through a memory or not

            int weight() const noexcept override {
                return flow;
            }
            Flow(int flow, bool stationair) : flow{ flow }, stationair{ stationair } {}
            ~Flow() {}
        };

        struct Time : public weighted_edge<float> { // Weighted by time the schedule takes to move the data
            std::string tau;
        };

        struct Space : public weighted_edge<float> { // Weighted by spatial 'width' the data occupies
            std::string s;
        };

        struct DeepLearningGraph {
            directed_graph<Operator, Flow> graph{};
            nodeId_t source{};
            nodeId_t sink{};
        };

        DeepLearningGraph create_dfa_graph() {
            // Create an directed graph to represent the Deep Learning domain flow graph
            directed_graph<Operator, Flow> dl_graph;

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
                const auto [source_id, target_id] {edge_id};
                ostr << "\t" << std::to_string(source_id) << " " << edge_specifier << " " << std::to_string(target_id) << " [" << edge_writer(edge_id, edge) << "]\n";
            }

            ostr << '\n';
        }

    }
}


int main() {
    using namespace sw::graph;
    const auto [graph, source, sink] {create_dfa_graph()};

    const std::filesystem::path& path{ "test_graph.json" }
    std::ofstream json_file{ path };

    return EXIT_SUCCESS;
}
