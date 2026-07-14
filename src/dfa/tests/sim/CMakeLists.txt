# SURE-parsing tests load their executable spec from docs/SURE so the doc stays
# the single source of truth.  Directory-scoped so every sim test (the growing
# linear-algebra operator catalog) picks up SURE_DOCS_DIR with no per-target
# wiring -- a new <op>_sure.cpp is a drop-in.
add_compile_definitions(SURE_DOCS_DIR="${DOMAINFLOW_ROOT}/docs/SURE")

file (GLOB SOURCES "./*.cpp")

compile_all("true" "test" "dfa/tests/sim" "${SOURCES}")
