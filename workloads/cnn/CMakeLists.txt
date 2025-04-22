file (GLOB SOURCES "./*.cpp")

compile_all("true"  "cnn"  "workloads/convolution"  "${SOURCES}")
