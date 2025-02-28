file (GLOB SOURCES "./*.cpp")

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
add_definitions(${LLVM_DEFINITIONS})

compile_all("true" "dfa" "dfa/tools" "${SOURCES}")


#set(DFATOOLS_CLI dfactl)
#add_executable(${DFATOOLS_CLI} dfa-cli.cpp)
#target_include_directories(${DFATOOLS_CLI} PUBLIC
#    ${DFATOOLS_ROOT}/include
#)
#target_link_libraries(${DFATOOLS_CLI} ${DFA_LIBRARY})  # Link to the DFA library
#set_target_properties(${DFATOOLS_CLI} PROPERTIES FOLDER "tools")


# Example: Add include directories specific to the tools (if needed)
# target_include_directories(MyExecutable PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/include  # For tool-specific headers
# )
