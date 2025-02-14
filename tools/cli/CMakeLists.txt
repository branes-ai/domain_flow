set(DFA_TOOL_CLI dfactl)
add_executable(${DFA_TOOL_CLI} dfa-cli.cpp)
target_link_libraries(${DFA_TOOL_CLI} ${DFA_LIBRARY})  # Link to the DFA library

# Tool 2 (Example - add more tools as needed)
# add_executable(AnotherTool AnotherTool.cpp)
# target_link_libraries(AnotherTool MyLibrary)

# Example: Add include directories specific to the tools (if needed)
# target_include_directories(MyExecutable PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/include  # For tool-specific headers
# )
