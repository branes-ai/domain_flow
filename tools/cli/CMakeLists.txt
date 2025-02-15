
set(DFATOOLS_CLI dfactl)
add_executable(${DFATOOLS_CLI} dfa-cli.cpp)
target_include_directories(${DFATOOLS_CLI} PUBLIC
    ${DFATOOLS_ROOT}/include
)

target_link_libraries(${DFATOOLS_CLI} ${DFA_LIBRARY})  # Link to the DFA library
set_target_properties(${DFATOOLS_CLI} PROPERTIES FOLDER "tools")

set(DFATOOLS_MATMUL matmul)
add_executable(${DFATOOLS_MATMUL} matmul.cpp)
target_include_directories(${DFATOOLS_MATMUL} PUBLIC
    ${DFATOOLS_ROOT}/include
)
target_link_libraries(${DFATOOLS_MATMUL} ${DFA_LIBRARY})  # Link to the DFA library
set_target_properties(${DFATOOLS_MATMUL} PROPERTIES FOLDER "tools")

# Example: Add include directories specific to the tools (if needed)
# target_include_directories(MyExecutable PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/include  # For tool-specific headers
# )
