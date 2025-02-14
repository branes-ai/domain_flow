cmake_minimum_required(VERSION 3.10)  # Or higher

project(dfa-tools)

# Set C++ standard (optional but recommended)
set(CMAKE_CXX_STANDARD 20)  # Or 14, 17, 20, 23, etc.
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DFA_TOOLS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# the domain flow library
add_subdirectory(src/dfa)

# the tools
add_subdirectory(tools)

# Optional: Install rules (for packaging)
install(TARGETS ${DFA_TOOL_CLI} DESTINATION bin)
install(TARGETS ${DFA_LIBRARY} DESTINATION lib)
install(FILES include/dfa/dfa.hpp DESTINATION include) # Install the header
