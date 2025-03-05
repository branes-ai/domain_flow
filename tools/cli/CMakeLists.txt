# file (GLOB SOURCES "./*.cpp")
set(SOURCES cli.cpp matmul.cpp)

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

compile_all("true" "dfa" "dfa/tools" "${SOURCES}")

set(DFA_MLIR_IMPORTER dfa_mlir)
set(DFA_MLIR_SRC mlir_txt_importer.cpp)
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
	MLIRFuncDialect 
	MLIRTosaDialect
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
set_target_properties(${DFA_MLIR_IMPORTER} PROPERTIES FOLDER "dfa/tools")

#set(DFA_MLIR_IMPORTER dfa_mlir)
#set(DFA_MLIR_SRC mlir_txt_importer.cpp)
#add_executable(${DFA_MLIR_IMPORTER} ${DFA_MLIR_SRC})
#message(STATUS "Add test ${DFA_MLIR_IMPORTER} from source ${DFA_MLIR_SRC}.")
#target_include_directories(${DFATOOLS_CLI} PUBLIC
#    ${DFATOOLS_ROOT}/include
#)
#target_link_libraries(${DFA_MLIR_IMPORTER} MLIRIR MLIRDialect MLIRParser MLIRSupport)  # Link to MLIR libs
#set_target_properties(${DFA_MLIR_IMPORTER} PROPERTIES FOLDER "dfa/tools")

# Example: Add include directories specific to the tools (if needed)
# target_include_directories(MyExecutable PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/include  # For tool-specific headers
# )
