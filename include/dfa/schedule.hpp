#pragma once
#include <string>
#include <vector>

#include <dfa/index_point.hpp>

namespace sw {
    namespace dfa {

        // Represents a specific schedule for the computation
        class Schedule {
        public:
            std::string name;
            std::vector<float> timing_vector;  // Coefficients for timing function
            std::vector<float> processor_vector;  // Coefficients for processor assignment
            
            Schedule(const std::string& n, 
                    const std::vector<float>& timing,
                    const std::vector<float>& proc)
                : name(n), timing_vector(timing), processor_vector(proc) {}
            
            // Calculate execution time for an index point
            float calculateExecutionTime(const IndexPoint& point) const {
                float time = 0.0f;
                for (size_t i = 0; i < point.coordinates.size(); ++i) {
                    time += timing_vector[i] * point.coordinates[i];
                }
                return time;
            }
            
            // Calculate processor assignment for an index point
            int calculateProcessor(const IndexPoint& point) const {
                float proc = 0.0f;
                for (size_t i = 0; i < point.coordinates.size(); ++i) {
                    proc += processor_vector[i] * point.coordinates[i];
                }
                return static_cast<int>(proc);
            }
        };

    }
}