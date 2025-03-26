//===- dfa_opt.cpp -------------------------------------*- C++ -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <iostream>
#include <filesystem>

#include "mlir/Config/mlir-config.h"
#include "mlir/IR/BuiltinOps.h"

#include "mlir/Pass/Pass.h"
#include "mlir/IR/Operation.h"
//#include "mlir/Dialect/LiteRT/IR/TFLOps.h"
//#include "llvm/Support/raw_ostream.h"


//#include "mlir/InitAllDialects.h"
//#include "mlir/Dialect/AMDGPU/IR/AMDGPUDialect.h"
//#include "mlir/Dialect/AMX/AMXDialect.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/IR/ValueBoundsOpInterfaceImpl.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Arith/IR/ValueBoundsOpInterfaceImpl.h"
#include "mlir/Dialect/Arith/Transforms/BufferDeallocationOpInterfaceImpl.h"
#include "mlir/Dialect/Arith/Transforms/BufferViewFlowOpInterfaceImpl.h"
#include "mlir/Dialect/Arith/Transforms/BufferizableOpInterfaceImpl.h"
//#include "mlir/Dialect/ArmNeon/ArmNeonDialect.h"
//#include "mlir/Dialect/ArmSME/IR/ArmSME.h"
//#include "mlir/Dialect/ArmSVE/IR/ArmSVEDialect.h"
#include "mlir/Dialect/Async/IR/Async.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/Bufferization/Transforms/FuncBufferizableOpInterfaceImpl.h"
//#include "mlir/Dialect/Complex/IR/Complex.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/ControlFlow/Transforms/BufferDeallocationOpInterfaceImpl.h"
#include "mlir/Dialect/ControlFlow/Transforms/BufferizableOpInterfaceImpl.h"
//#include "mlir/Dialect/DLTI/DLTI.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
//#include "mlir/Dialect/GPU/IR/GPUDialect.h"
//#include "mlir/Dialect/GPU/Transforms/BufferDeallocationOpInterfaceImpl.h"
//#include "mlir/Dialect/IRDL/IR/IRDL.h"
//#include "mlir/Dialect/Index/IR/IndexDialect.h"
//#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
//#include "mlir/Dialect/LLVMIR/NVVMDialect.h"
//#include "mlir/Dialect/LLVMIR/ROCDLDialect.h"
//#include "mlir/Dialect/LLVMIR/Transforms/InlinerInterfaceImpl.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/Linalg/Transforms/AllInterfaces.h"
#include "mlir/Dialect/Linalg/Transforms/RuntimeOpVerification.h"
//#include "mlir/Dialect/MLProgram/IR/MLProgram.h"
//#include "mlir/Dialect/MLProgram/Transforms/BufferizableOpInterfaceImpl.h"
//#include "mlir/Dialect/MPI/IR/MPI.h"
#include "mlir/Dialect/Math/IR/Math.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/MemRef/IR/MemRefMemorySlot.h"
#include "mlir/Dialect/MemRef/IR/ValueBoundsOpInterfaceImpl.h"
#include "mlir/Dialect/MemRef/Transforms/AllocationOpInterfaceImpl.h"
#include "mlir/Dialect/MemRef/Transforms/BufferViewFlowOpInterfaceImpl.h"
#include "mlir/Dialect/MemRef/Transforms/RuntimeOpVerification.h"
//#include "mlir/Dialect/Mesh/IR/MeshDialect.h"
//#include "mlir/Dialect/NVGPU/IR/NVGPUDialect.h"
//#include "mlir/Dialect/OpenACC/OpenACC.h"
//#include "mlir/Dialect/OpenMP/OpenMPDialect.h"
//#include "mlir/Dialect/PDL/IR/PDL.h"
//#include "mlir/Dialect/PDLInterp/IR/PDLInterp.h"
//#include "mlir/Dialect/Polynomial/IR/PolynomialDialect.h"
//#include "mlir/Dialect/Ptr/IR/PtrDialect.h"
//#include "mlir/Dialect/Quant/IR/Quant.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/SCF/IR/ValueBoundsOpInterfaceImpl.h"
#include "mlir/Dialect/SCF/TransformOps/SCFTransformOps.h"
#include "mlir/Dialect/SCF/Transforms/BufferDeallocationOpInterfaceImpl.h"
#include "mlir/Dialect/SCF/Transforms/BufferizableOpInterfaceImpl.h"
#include "mlir/Dialect/SPIRV/IR/SPIRVDialect.h"
#include "mlir/Dialect/Shape/IR/Shape.h"
#include "mlir/Dialect/Shape/Transforms/BufferizableOpInterfaceImpl.h"
#include "mlir/Dialect/SparseTensor/IR/SparseTensor.h"
#include "mlir/Dialect/SparseTensor/Transforms/BufferizableOpInterfaceImpl.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Dialect/Tensor/IR/TensorInferTypeOpInterfaceImpl.h"
#include "mlir/Dialect/Tensor/IR/TensorTilingInterfaceImpl.h"
#include "mlir/Dialect/Tensor/IR/ValueBoundsOpInterfaceImpl.h"
#include "mlir/Dialect/Tensor/TransformOps/TensorTransformOps.h"
#include "mlir/Dialect/Tensor/Transforms/BufferizableOpInterfaceImpl.h"
#include "mlir/Dialect/Tensor/Transforms/SubsetInsertionOpInterfaceImpl.h"
#include "mlir/Dialect/Tosa/IR/ShardingInterfaceImpl.h"
#include "mlir/Dialect/Tosa/IR/TosaOps.h"
#include "mlir/Dialect/Transform/IR/TransformDialect.h"
#include "mlir/Dialect/Transform/PDLExtension/PDLExtension.h"
//#include "mlir/Dialect/UB/IR/UBOps.h"
#include "mlir/Dialect/Vector/IR/ValueBoundsOpInterfaceImpl.h"
#include "mlir/Dialect/Vector/IR/VectorOps.h"
#include "mlir/Dialect/Vector/Transforms/BufferizableOpInterfaceImpl.h"
#include "mlir/Dialect/Vector/Transforms/SubsetOpInterfaceImpl.h"
//#include "mlir/Dialect/X86Vector/X86VectorDialect.h"
//#include "mlir/Dialect/XeGPU/IR/XeGPU.h"


//#include <mlir/InitAllExtensions.h>

#include "mlir/Tools/mlir-opt/MlirOptMain.h"

namespace mlir {
	/// add the MLIR dialects that collaborate with Domain Flow
	inline void registerSupportingDialects(DialectRegistry& registry) {
		// clang-format off
		registry.insert<affine::AffineDialect,
						arith::ArithDialect,
						async::AsyncDialect,
						bufferization::BufferizationDialect,
						cf::ControlFlowDialect,
						emitc::EmitCDialect,
						func::FuncDialect,
						linalg::LinalgDialect,
						math::MathDialect,
						memref::MemRefDialect,
						scf::SCFDialect,
						spirv::SPIRVDialect,
						shape::ShapeDialect,
						sparse_tensor::SparseTensorDialect,
						tensor::TensorDialect,
						tosa::TosaDialect,
						transform::TransformDialect,
						vector::VectorDialect
						>();
		// clang-format on

		// register all the external models
		affine::registerValueBoundsOpInterfaceExternalModels(registry);
		// which library provides these external models?
		//arith::registerBufferDeallocationOpInterfaceExternalModels(registry);
		//arith::registerBufferizableOpInterfaceExternalModels(registry);
		//arith::registerBufferViewFlowOpInterfaceExternalModels(registry);
		//arith::registerValueBoundsOpInterfaceExternalModels(registry);
		//bufferization::func_ext::registerBufferizableOpInterfaceExternalModels(registry);
		//cf::registerBufferizableOpInterfaceExternalModels(registry);
		//cf::registerBufferDeallocationOpInterfaceExternalModels(registry);
	}


	struct ComplexityAnalysisPass : public PassWrapper<ComplexityAnalysisPass, OperationPass<mlir::func::FuncOp>> {
		MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(ComplexityAnalysisPass)

			void runOnOperation() override {
			mlir::func::FuncOp funcOp = getOperation();
			funcOp.walk([&](mlir::Operation* op) {
				if (auto convOp = mlir::dyn_cast<tosa::Conv2DOp>(op)) {
					// Example: Calculate complexity for Conv2D
					llvm::outs() << "Conv2D Op Found: " << op->getName() << "\n";
					//Accessing the operands of the convolution operation.
					auto inputShape = convOp.getInput().getType().cast<mlir::TensorType>().getShape();
					//auto filterShape = convOp.getFilter().getType().cast<mlir::TensorType>().getShape();

					//Simple example of calculating complexity.
					long long complexity = 1;
					for (auto dim : inputShape) {
						complexity *= dim;
					}
					//for (auto dim : filterShape) {
					//	complexity *= dim;
					//}
					llvm::outs() << "Estimated Complexity: " << complexity << "\n";
				}
				// Add logic for other TFLite ops (e.g., Add, MatMul)
				});
		}
	};

	std::unique_ptr<mlir::Pass> createComplexityAnalysisPass() {
		return std::make_unique<ComplexityAnalysisPass>();
	}

	//-----------------------------------------------------------------------------------------
	// Define the pass
	class ArithmeticComplexityPass
		: public PassWrapper<ArithmeticComplexityPass, OperationPass<ModuleOp>> {
	public:
		StringRef getArgument() const override { return "arithmetic-complexity"; } // command line switch to run the pass
		StringRef getDescription() const override {	return "Computes arithmetic complexity of the MLIR graph."; }

		void runOnOperation() override {
			ModuleOp module = getOperation();

			// Initialize counters
			int64_t addCount = 0, mulCount = 0, divCount = 0, totalOps = 0;

			// Traverse operations
			module.walk([&](Operation* op) {
				llvm::outs() << op->getName() << '\n';
				//if (isa<AddFOp>(op)) {
				//	addCount++;
				//}
				//else if (isa<MulFOp>(op)) {
				//	mulCount++;
				//}
				//else if (isa<DivFOp>(op)) {
				//	divCount++;
				//}
				// Count all operations
				totalOps++;
				});

			// Print the results
			llvm::outs() << "Arithmetic Complexity Report:\n";
			llvm::outs() << "Add operations: " << addCount << "\n";
			llvm::outs() << "Mul operations: " << mulCount << "\n";
			llvm::outs() << "Div operations: " << divCount << "\n";
			llvm::outs() << "Total operations: " << totalOps << "\n";
		}
	};

	// Register the pass
	std::unique_ptr<Pass> createArithmeticComplexityPass() {
		return std::make_unique<ArithmeticComplexityPass>();
	}

	static PassRegistration<ArithmeticComplexityPass> pass;

	// op->setAttr("complexity", IntegerAttr::get(IntegerType::get(op->getContext(), 64), opComplexity));

}

/// This test includes the minimal amount of components for dfa-opt, that is
/// the CoreIR, the printer/parser, the bytecode reader/writer, the
/// passmanagement infrastructure and all the instrumentation.
int main(int argc, char **argv) {
	std::cout << "Current path is " << std::filesystem::current_path() << '\n';
	mlir::DialectRegistry registry;
	mlir::registerSupportingDialects(registry);
	return mlir::asMainReturnCode(mlir::MlirOptMain(argc, argv, "Domain flow optimizer driver\n", registry));
}
