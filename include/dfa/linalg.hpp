#pragma once
#include <dfa/vector.hpp>
#include <dfa/matrix.hpp>

namespace sw {
    namespace dfa {

        // Helper function to normalize a vector
		template<typename Scalar>
        Vector3<Scalar> normalize(const Vector3<Scalar>& v) {
			auto x = v[0];
			auto y = v[1];
			auto z = v[2];
            float magnitude = std::sqrt(x*x + y*y + z*z);
            if (magnitude == 0.0f) {
                return { 0.0f, 0.0f, 0.0f }; // Avoid division by zero
            }
            return { x / magnitude, y / magnitude, z / magnitude };
        }

        // Helper function for cross product
        template<typename Scalar>
        Vector3<Scalar> cross(const Vector3<Scalar>& a, const Vector3<Scalar>& b) {
			auto ax = a[0]; auto ay = a[1]; auto az = a[2];
			auto bx = b[0]; auto by = b[1]; auto bz = b[2];
			return { ay * bz - az * by, az * bx - ax * bz, ax * by - ay * bx };
        }

        // Helper function for dot product
        template<typename Scalar>
        Scalar dot(const Vector3<Scalar>& a, const Vector3<Scalar>& b) {
            return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
        }

        // Function to calculate the rotation matrix from one vector to another
        template<typename Scalar>
        Matrix3<Scalar> computeRotationFromTwoVectors(const Vector3<Scalar>& start, const Vector3<Scalar>& end) {
            Vector3<Scalar> a = normalize(start);
            Vector3<Scalar> b = normalize(end);

            Vector3<Scalar> v = cross(a, b);  // axis of rotation: v is perpendicular to both a and b
            Scalar c = dot(a, b);
            Scalar s = std::sqrt(dot(v, v));

            // Handle the case where the vectors are collinear
            if (s < 1e-6) {
				Matrix3<Scalar> identity; // default constructor creates identity matrix
                if (c < 0) {
                    // 180 degree rotation around an arbitrary axis perpendicular to both
                    Vector3<Scalar> arbitraryAxis;
                    if (std::abs(a[0]) < 0.9) {
                        arbitraryAxis = normalize<Scalar>({ Scalar(1.0), Scalar(0.0), Scalar(0.0) });
                    }
                    else if (std::abs(a[1]) < 0.9) {
                        arbitraryAxis = normalize<Scalar>({ Scalar(0.0), Scalar(1.0), Scalar(0.0) });
                    }
                    else {
                        arbitraryAxis = normalize<Scalar>({ Scalar(0.0), Scalar(0.0), Scalar(1.0) });
                    }
                    Vector3<Scalar> rotationAxis = normalize(cross(a, arbitraryAxis));
                    Scalar x = rotationAxis[0];
                    Scalar y = rotationAxis[1];
                    Scalar z = rotationAxis[2];
					// Rodrigues' rotation formula for 180 degrees
					// https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
					Scalar Rxx = 1.0 - 2.0 * y * y - 2.0 * z * z;
					Scalar Rxy = 2.0 * x * y + 2.0 * z;
					Scalar Rxz = 2.0 * x * z - 2.0 * y;
					Scalar Ryx = 2.0 * x * y - 2.0 * z;
					Scalar Ryy = 1.0 - 2.0f * x * x - 2.0 * z * z;
					Scalar Ryz = 2.0 * y * z + 2.0 * x;
					Scalar Rzx = 2.0 * x * z + 2.0 * y;
					Scalar Rzy = 2.0 * y * z - 2.0 * x;
					Scalar Rzz = 1.0 - 2.0 * x * x - 2.0 * y * y;
					// Construct the rotation matrix
					return { {Rxx, Rxy, Rxz},
							 {Ryx, Ryy, Ryz},
							 {Rzx, Rzy, Rzz} };
                }
                return identity;
            }

            // Construct the skew-symmetric cross-product matrix [v]x
            Matrix3<Scalar> vx = { 
                                    { Scalar(0.0),       -v[2],        v[1]},
                                    {        v[2], Scalar(0.0),       -v[0]},
                                    {       -v[1],        v[0], Scalar(0.0)}
                                 };

            Matrix3<Scalar> vx_sq;
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    for (int k = 0; k < 3; ++k) {
                        vx_sq[i][j] += vx[i][k] * vx[k][j];
                    }
                }
            }

            //Matrix3<Scalar> identity;
            //// Rodrigues' rotation formula
            //Matrix3<Scalar> rotationMatrix;
            //for (int i = 0; i < 3; ++i) {
            //    for (int j = 0; j < 3; ++j) {
            //        rotationMatrix[i][j] = identity[i][j] + (Scalar(1.0) - c) / (s * s) * vx_sq[i][j] + (Scalar(1.0) / s) * vx[i][j];
            //    }
            //}

            // Normalize the rotation axis v
            Vector3<Scalar> rotationAxisNormalized = normalize(v);
            Scalar x = rotationAxisNormalized[0];
            Scalar y = rotationAxisNormalized[1];
            Scalar z = rotationAxisNormalized[2];

            // Construct the rotation matrix using the angle and normalized axis
            Matrix3 rotationMatrix = {
                {c + x * x * (1 - c), x * y * (1 - c) - z * s, x * z * (1 - c) + y * s},
                {y * x * (1 - c) + z * s, c + y * y * (1 - c), y * z * (1 - c) - x * s},
                {z * x * (1 - c) - y * s, z * y * (1 - c) + x * s, c + z * z * (1 - c)}
            };


            return rotationMatrix;
        }

    }
}
