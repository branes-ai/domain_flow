file (GLOB SOURCES "./*.cpp")

compile_all("true"  "dse"  "tools/dse"  "${SOURCES}")

add_subdirectory("tests")
