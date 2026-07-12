file (GLOB SOURCES "./*.cpp")

compile_all("true" "test" "dfa/tests/sim" "${SOURCES}")

# the SURE parser test loads the canonical example from docs/SURE so the
# executable doc stays the single source of truth
target_compile_definitions(test_sure_parser PRIVATE
    SURE_DOCS_DIR="${DOMAINFLOW_ROOT}/docs/SURE")
