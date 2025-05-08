#pragma once
#include<iostream>
#include<iomanip>
#include<array>

namespace sw {
    namespace dfa {

        class DomainFlow {
        public:
            using ConstraintCoefficientType = int;
            using CCType = ConstraintCoefficientType;
            using VertexId = size_t;
            using FaceId = size_t;

            // Represents a pair of faces (input and output) for alignment
            struct FacePair {
                size_t source_hull_idx; // Index of the source hull
                size_t target_hull_idx; // Index of the target hull
                FaceId source_face_id;  // ID of the source face
                FaceId target_face_id;  // ID of the target face
                /*               FacePair(size_t srcHullIdx, size_t tgtHullIdx, size_t srcFaceId, size_t tgtFaceId)
                                   : source_hull_idx(srcHullIdx), target_hull_idx(tgtHullIdx),
                                   source_face_id(srcFaceId), target_face_id(tgtFaceId) {
                               }*/
            };

            struct Transform {
//                Matrix4 matrix; // 4x4 homogeneous transformation matrix
                FacePair connection;
            };

        public:
            void addConvexHull(const ConvexHull<CCType>& hull) {
                convex_hulls.push_back(hull);
            }

            void alignHulls() {
                for (auto& ch : convex_hulls) {
                    std::cout << ch << '\n';
                }
            }

        private:
            std::vector<ConvexHull<CCType>> convex_hulls;

            friend inline std::ostream& operator<<(std::ostream& os, const DomainFlow& df) {
                os << "Domain Flow Space:\n";
                for (const auto& ch : df.convex_hulls) {
                    os << "  " << ch << '\n';
                }
                return os;
            }
        };

    }
}
