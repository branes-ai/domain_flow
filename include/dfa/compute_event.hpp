#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <dfa/indexspace.hpp>

namespace sw {
    namespace dfa {
        
        // Represents a dependency between compute events
        class DependencyEdge {
        public:
            std::weak_ptr<ComputeEvent> source;
            std::weak_ptr<ComputeEvent> target;
            std::vector<int> dependency_vector;  // Dependency distance vector
            
            DependencyEdge(const std::shared_ptr<ComputeEvent>& src, 
                        const std::shared_ptr<ComputeEvent>& tgt,
                        const std::vector<int>& dep_vector)
                : dependency_vector(dep_vector) {
                source = src;
                target = tgt;
            }
        };
        
        // Represents a computational event in the parallel algorithm
        class ComputeEvent {
        public:
            IndexPoint index_point;
            glm::vec4 position;  // Position in visualization space
            std::vector<std::shared_ptr<DependencyEdge>> incoming_edges;
            std::vector<std::shared_ptr<DependencyEdge>> outgoing_edges;
            float execution_time;  // Scheduled execution time
            int processor_assignment;  // Assigned processor ID
            
            ComputeEvent(const IndexPoint& point) : index_point(point), 
                                                position(glm::vec4(0.0f)),
                                                execution_time(0.0f),
                                                processor_assignment(-1) {}
            
            // Add dependency edge
            void addDependency(const std::shared_ptr<DependencyEdge>& edge, bool is_incoming) {
                if (is_incoming) {
                    incoming_edges.push_back(edge);
                } else {
                    outgoing_edges.push_back(edge);
                }
            }
        };

    }
}