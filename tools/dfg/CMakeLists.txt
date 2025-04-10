file (GLOB SOURCES "./*.cpp")

compile_all("true"  "dfg"  "tools/dfg"  "${SOURCES}")

add_subdirectory("tests")
