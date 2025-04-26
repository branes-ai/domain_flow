#pragma once


namespace sw {
    namespace dfa {



		struct shapeAnalysisResults {
			int64_t batchSize, m, k, n;
			std::string errMsg;
			void clear() {
				batchSize = 1;
				m = 0;
				k = 0;
				n = 0;
				errMsg.clear();
			}
			void setError(const std::string& errMsg) { this->errMsg = errMsg; }
			uint64_t macOps() const {
				return batchSize * m * n * k;
			}
		};

		/// <summary>
		/// shape analysis: for numpy matmul, the last two dimensions are the matmul dimensions, leading dimensions are batch dimensions
		/// (b1, b2, ..., bn, m, k) * (b1, b2, ..., bn, k, n) yields (b1, b2, ..., bn, m, n)
		/// </summary>
		/// <param name="shape0"></param>
		/// <param name="shape1"></param>
		/// <returns>true if analysis determines there are no inconsistencies, false otherwise</returns>
		inline bool calculateMatmulShape(const std::vector<int>& lhsShape, const std::vector<int>& rhsShape, shapeAnalysisResults& result) {
			result.clear();
			// Ensure rhs tensor has at least 2 dimensions
			if (rhsShape.size() < 2) {
				result.setError("Right tensor must have at least 2 dimensions.");
				return false;
			}

			// Ensure lhs tensor has at least 1 dimension
			if (lhsShape.empty()) {
				result.setError("Left tensor must have at least 1 dimension.");
				return false;
			}

			// Extract k, n from rhs tensor
			int k1 = rhsShape[rhsShape.size() - 2]; // Rows of right matrix
			int n = rhsShape[rhsShape.size() - 1];  // Columns of right matrix

			// Determine batch dimensions from shape1
			size_t batch_dims = rhsShape.size() - 2; // Number of batch dimensions

			// Determine if shape0 is a vector or matrix
			bool is_vector = (lhsShape.size() == batch_dims + 1);
			bool is_matrix = (lhsShape.size() == batch_dims + 2);

			// Validate shape0 size
			if (!is_vector && !is_matrix) {
				result.setError("Left tensor has incompatible number of dimensions.");
				return false;
			}

			// Extract m, k from lhs tensor
			int m, k0;
			if (lhsShape.size() == 1) {
				// Vector case: lhs shape = (batch_size,)
				m = 1;
				k0 = lhsShape[0];
			}
			else {
				// Matrix case: lhs shape = (..., m, k)
				m = lhsShape[lhsShape.size() - 2];
				k0 = lhsShape[lhsShape.size() - 1];
			}

			// Validate that reduction dimensions match
			if (k0 != k1) {
				result.setError("Inner dimensions must match: k0 != k1");
				return false;
			}
			int k = k0;

			// Check batch dimensions
			uint64_t batch_size = 1;
			for (size_t i = 0; i < batch_dims; ++i) {
				if (i >= lhsShape.size()) {
					result.setError("Left tensor has too few dimensions for batch.");
					return false;
				}
				if (lhsShape[i] != rhsShape[i]) {
					result.setError("Batch dimensions must match.");
					return false;
				}
				if (lhsShape[i] <= 0) {
					result.setError("Dimensions must be positive.");
					return false;
				}
				batch_size *= static_cast<uint64_t>(lhsShape[i]);
			}

			// Validate matrix/vector dimensions
			if (m <= 0 || k <= 0 || n <= 0) {
				result.setError("Matrix dimensions must be positive.");
				return false;
			}

			// Total MAC operations = batch_size * m * n * k
			result.batchSize = batch_size;
			result.m = m;
			result.k = k;
			result.n = n;
			return true;
		}


    }
}


