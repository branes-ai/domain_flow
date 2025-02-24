
set(DFA_SIMULATOR dfactl)
add_executable(${DFA_SIMULATOR} dfa-cli.cpp)
target_include_directories(${DFA_SIMULATOR} PUBLIC
    ${DFATOOLS_ROOT}/include
)
target_link_libraries(${DFA_SIMULATOR} ${DFA_LIBRARY})  # Link to the DFA library
set_target_properties(${DFA_SIMULATOR} PROPERTIES FOLDER "tools")


# Example: Add include directories specific to the tools (if needed)
# target_include_directories(MyExecutable PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/include  # For tool-specific headers
# )
