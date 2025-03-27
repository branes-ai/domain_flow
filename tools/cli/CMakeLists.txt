####
# Set dfa tools specific include directories
find_package(MLIR REQUIRED CONFIG)

message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/lib)
set(MLIR_BINARY_DIR ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(HandleLLVMOptions)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})
link_directories(${LLVM_BUILD_LIBRARY_DIR})
link_directories(${MLIR_BUILD_LIBRARY_DIR})
add_definitions(${LLVM_DEFINITIONS})

set(LIBS
	LLVMCore 
	LLVMSupport 
	MLIRIR 
	MLIRDialect 
	MLIRParser 
	MLIRPass
	MLIRTransforms
	MLIRSupport 
	MLIRAsmParser 

	MLIRAsyncDialect
	MLIRBufferizationDialect
	MLIRControlFlowDialect
	MLIREmitCDialect
	MLIRFuncDialect
	MLIRLinalgDialect
	MLIRMathDialect
	MLIRMemRefDialect
	MLIRSCFDialect
	MLIRSPIRVDialect
	MLIRShapeDialect
	MLIRSparseTensorDialect
	MLIRTensorDialect
	MLIRTosaDialect
	MLIRTransformDialect
	MLIRVectorDialect

	MLIROptLib
)

###################################################
# define the dfa-opt tool
set(DFA_OPT dfa-opt)
set(DFA_OPT_SRC dfa_opt.cpp)
add_mlir_tool(${DFA_OPT}
  ${DFA_OPT_SRC}
  PARTIAL_SOURCES_INTENDED

  DEPENDS
  ${LIBS}
  )
target_link_libraries(${DFA_OPT} PRIVATE ${LIBS})
llvm_update_compile_flags(${DFA_OPT})
mlir_check_all_link_libraries(${DFA_OPT})
set_target_properties(${DFA_OPT} PROPERTIES FOLDER "dfa/tools/opt")

