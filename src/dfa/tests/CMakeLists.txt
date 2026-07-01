file (GLOB SOURCES "./*.cpp")

compile_all("true" "test" "dfa/tests" "${SOURCES}")

add_subdirectory(sim)
