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

set(DFA_MLIR_IMPORTER dfa_mlir)
set(DFA_MLIR_SRC mlir_importer.cpp)
#add_executable(${DFA_MLIR_IMPORTER} ${DFA_MLIR_SRC})
#message(STATUS "Add test ${DFA_MLIR_IMPORTER} from source ${DFA_MLIR_SRC}.")
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

add_mlir_tool(${DFA_MLIR_IMPORTER} 
	${DFA_MLIR_SRC}
	PARTIAL_SOURCES_INTENDED

	DEPENDS
	${LIBS}
)
target_link_libraries(${DFA_MLIR_IMPORTER} PRIVATE ${LIBS})
llvm_update_compile_flags(${DFA_MLIR_IMPORTER})
mlir_check_all_link_libraries(${DFA_MLIR_IMPORTER})
set_target_properties(${DFA_MLIR_IMPORTER} PROPERTIES FOLDER "dfa/tools/mlir")

