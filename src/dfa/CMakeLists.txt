set(DFA_LIBRARY dfalib)
file(GLOB SRCS *.cpp *.hpp)

add_library(${DFA_LIBRARY} ${SRCS})

target_include_directories(${DFA_LIBRARY} PUBLIC
    ${DFA_TOOLS_ROOT}/include
)

target_compile_definitions(${DFA_LIBRARY} PUBLIC
    MY_LIBRARY_DEFINITION
)

# Set library version
set_target_properties(${DFA_LIBRARY} PROPERTIES
    VERSION 0.1.0
)


# setup the tests
add_subdirectory(tests)
