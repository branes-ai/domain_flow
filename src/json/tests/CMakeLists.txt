set(JSON_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/vendor/nlohmann/single_include")
message(STATUS "JSON INCLUDE DIR: ${JSON_INCLUDE_DIR}")
include_directories(${JSON_INCLUDE_DIR})
file (GLOB SOURCES "./*.cpp")

compile_all("true" "test" "json/tests" "${SOURCES}")
