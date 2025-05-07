#pragma once
#include <dfa/vector.hpp>
#include <dfa/matrix.hpp>

namespace sw {
    namespace dfa {

        // Compute rotation matrix for angle theta around axis v (Rodrigues' formula)
        Matrix3d angleAxisRotation(double theta, const Vector3d& v) {
            Matrix3d R;
            double c = std::cos(theta);
            double s = std::sin(theta);
            double t = 1.0 - c;
            double vx = v.x, vy = v.y, vz = v.z;

            // Rodrigues' rotation matrix
            R.data = {
                {c + vx * vx * t,        vx * vy * t - vz * s,   vx * vz * t + vy * s},
                {vy * vx * t + vz * s,   c + vy * vy * t,        vy * vz * t - vx * s},
                {vz * vx * t - vy * s,   vz * vy * t + vx * s,   c + vz * vz * t}
            };

            return R;
        }

        // Compute rotation matrix to align source vector to target vector
        Matrix3d computeRotation(const Vector3d& source, const Vector3d& target) {
            Vector3d v1 = source.normalized();
            Vector3d v2 = target.normalized();

            // Check if vectors are parallel or opposite
            if ((v1 - v2).norm() < 1e-6) return Matrix3d::identity();
            if ((v1 + v2).norm() < 1e-6) {
                // 180-degree rotation around x-axis
                Matrix3d m;
                m.data = {
                    {-1, 0, 0},
                    {0, -1, 0},
                    {0, 0, 1}
                };
                return m;
            }

            // Compute rotation axis and angle
            Vector3d axis = v1.cross(v2).normalized();
            double angle = std::acos(v1.dot(v2));

            // Compute rotation matrix using angle-axis (Rodrigues' formula)
            return angleAxisRotation(angle, axis);
        }



    }
}
