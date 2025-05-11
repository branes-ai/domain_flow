
namespace sw {
    namespace experimental {

        // ConvexHull class
		template<typename ConstraintCoefficientType = int>
        class ConvexHull {
			using CCType = ConstraintCoefficientType;
			using MatX = MatrixX<CCType>;
			using VecX = VectorX<CCType>;
			using Mat3 = Matrix3<CCType>;
			using Vec3 = Vector3<CCType>;
			using Mat4 = Matrix4<CCType>;
        public:
            ConvexHull(const MatX& A, const VecX& b, double m, double n, double k)
                : A_(A), b_(b), m_(m), n_(n), k_(k) {
                if (A_.rows() != b_.size() || A_.cols() != 3)
                    throw std::invalid_argument("Invalid constraint dimensions");
            }

            const MatX& getA() const { return A_; }
            const VecX& getB() const { return b_; }

            ConvexHull transform(const Mat3& R, const Vec3& t) const {
                MatX A_new = A_ * R.transpose();
                VecX b_new(b_.size());
                for (int i = 0; i < b_.size(); ++i)
                    b_new(i) = b_(i) - A_.row(i).dot(t);
                return ConvexHull(A_new, b_new, m_, n_, k_);
            }

            std::vector<Vec3> getVertices() const {
                return {
                    Vec3(0, 0, 0),      // v0
                    Vec3(m_, 0, 0),     // v1
                    Vec3(m_, 0, k_),    // v2
                    Vec3(0, 0, k_),     // v3
                    Vec3(0, n_, k_),    // v4
                    Vec3(0, n_, 0),     // v5
                    Vec3(m_, n_, 0),    // v6
                    Vec3(m_, n_, k_)    // v7
                };
            }

            std::vector<Vec3> getTransformedVertices(const Mat4& T) const {
                std::vector<Vec3> vertices = getVertices();
                std::vector<Vec3> transformed;
                for (const auto& v : vertices) {
                    transformed.push_back(T.transformPoint(v));
                }
                return transformed;
            }

        private:
            MatX A_;
            VecX b_;
            CCType m_, n_, k_;
        };



    }
}