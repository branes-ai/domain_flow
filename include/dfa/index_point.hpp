#pragma once
#include <vector>

namespace sw {
    namespace dfa {

        // Represents a point in the index space of the SARE
        struct IndexPoint {
            std::vector<int> coordinates;  // N-dimensional coordinates
            
			// set an index point to a specific value
			IndexPoint(int dimension, int value = 0) : coordinates(dimension, value) {}
            // Initializer list constructor
            IndexPoint(std::initializer_list<int> i) : coordinates(i) {}
            // Constructor for arbitrary dimensions
            IndexPoint(const std::vector<int>& coords) : coordinates(coords) {}
            
			int& operator[](size_t index) {
				return coordinates[index];
			}
			int operator[](size_t index) const {
				return coordinates[index];
			}

            // Comparison operators for use in containers
            inline bool operator==(const IndexPoint& other) const {
                return coordinates == other.coordinates;
            }
            inline bool operator<(const IndexPoint& other) const {
                return coordinates < other.coordinates;
            }
        };

        inline std::ostream& operator<<(std::ostream& ostr, const IndexPoint& p) {
            size_t dim = p.coordinates.size();
            ostr << '(';
            for (size_t i = 0; i < dim; ++i) {
                ostr << p.coordinates[i];
                if (i < p.coordinates.size() - 1) {
                    ostr << ", ";
                }
            }
            return ostr << ") ";
        }

    }
}

// Custom hash function for IndexPoint
namespace std {
    template<>
    struct hash<sw::dfa::IndexPoint> {
        size_t operator()(const sw::dfa::IndexPoint& p) const {
            size_t hash = 0;
            for (const auto& coord : p.coordinates) {
                hash ^= std::hash<int>()(coord) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
    };
}