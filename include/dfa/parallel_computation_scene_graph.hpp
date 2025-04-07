#pragma once
#include <vector>
#include <map>
#include <unordered_map>
#include <dfa/index_point.hpp>

namespace sw {
    namespace dfa {

        // Main scene graph class for the visualization
        class ParallelComputationSceneGraph {
        private:
            std::unordered_map<IndexPoint, std::shared_ptr<ComputeEvent>> events;
            std::vector<std::shared_ptr<DependencyEdge>> dependencies;
            std::vector<Schedule> schedules;
            int dimensions;  // Dimensionality of the index space
        
        public:
            ParallelComputationSceneGraph(int dims) : dimensions(dims) {}
            
            // Add a compute event to the scene graph
            std::shared_ptr<ComputeEvent> addComputeEvent(const IndexPoint& point) {
                auto event = std::make_shared<ComputeEvent>(point);
                events[point] = event;
                return event;
            }
            
            // Add a dependency between two events
            void addDependency(const IndexPoint& source, 
                            const IndexPoint& target,
                            const std::vector<int>& dep_vector) {
                auto source_event = events[source];
                auto target_event = events[target];
                
                auto edge = std::make_shared<DependencyEdge>(source_event, target_event, dep_vector);
                source_event->addDependency(edge, false);
                target_event->addDependency(edge, true);
                dependencies.push_back(edge);
            }
            
            // Add a new schedule
            void addSchedule(const std::string& name,
                            const std::vector<float>& timing_vector,
                            const std::vector<float>& processor_vector) {
                schedules.emplace_back(name, timing_vector, processor_vector);
            }
            
            // Apply a schedule to all events
            void applySchedule(size_t schedule_index) {
                if (schedule_index >= schedules.size()) return;
                
                const auto& schedule = schedules[schedule_index];
                for (auto& [point, event] : events) {
                    event->execution_time = schedule.calculateExecutionTime(point);
                    event->processor_assignment = schedule.calculateProcessor(point);
                    
                    // Update visualization position based on schedule
                    // This is a simple mapping - you might want to customize this
                    for (int i = 0; i < dimensions; ++i) {
                        event->position[i] = static_cast<float>(point.coordinates[i]);
                    }
                    event->position[dimensions] = event->execution_time;  // Use time as an additional dimension
                }
            }
            
            // Get all events within a specific time window
            std::vector<std::shared_ptr<ComputeEvent>> getEventsInTimeWindow(float start_time, float end_time) {
                std::vector<std::shared_ptr<ComputeEvent>> result;
                for (const auto& [point, event] : events) {
                    if (event->execution_time >= start_time && event->execution_time <= end_time) {
                        result.push_back(event);
                    }
                }
                return result;
            }
        };

    }
}