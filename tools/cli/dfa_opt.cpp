//===- dfa_opt.cpp -------------------------------------*- C++ -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "mlir/Config/mlir-config.h"

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
}
/// This test includes the minimal amount of components for dfa-opt, that is
/// the CoreIR, the printer/parser, the bytecode reader/writer, the
/// passmanagement infrastructure and all the instrumentation.
int main(int argc, char **argv) {
  mlir::DialectRegistry registry;
  mlir::registerSupportingDialects(registry);
  return mlir::asMainReturnCode(mlir::MlirOptMain(argc, argv, "Domain flow optimizer driver\n", registry));
}
