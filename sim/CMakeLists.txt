# dfactl: the Domain Flow Architecture SURE simulator CLI
set(DFA_SIMULATOR dfactl)
add_executable(${DFA_SIMULATOR} dfa-cli.cpp)

# domain_flow is the header-only INTERFACE library (defined in the top-level
# CMakeLists); linking it supplies the include/ search path and C++20.
target_link_libraries(${DFA_SIMULATOR} PRIVATE ${PROJECT_NAME})

set_target_properties(${DFA_SIMULATOR} PROPERTIES FOLDER "tools")

# Publish the CLI target name to the parent scope so the top-level install rule
# can package it (see install(TARGETS ${DFA_TOOL_CLI} ...)).
set(DFA_TOOL_CLI ${DFA_SIMULATOR} PARENT_SCOPE)

if(DOMAINFLOW_BUILD_TESTING)
    add_test(NAME dfactl_matmul COMMAND ${DFA_SIMULATOR} matmul)
    add_test(NAME dfactl_matvec COMMAND ${DFA_SIMULATOR} matvec)
    add_test(NAME dfactl_qr     COMMAND ${DFA_SIMULATOR} qr)
    # the executable version of docs/SURE/matmul.md through the DSL front-end
    add_test(NAME dfactl_sure_matmul
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/matmul.sure --schedule linear)
    add_test(NAME dfactl_sure_qr
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/qr.sure)
    add_test(NAME dfactl_sure_conv2d
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/conv2d.sure)
    add_test(NAME dfactl_sure_axpy
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/axpy.sure)
    add_test(NAME dfactl_sure_dot
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/dot.sure)
    add_test(NAME dfactl_sure_nrm2
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/nrm2.sure)
    add_test(NAME dfactl_sure_asum
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/asum.sure)
    add_test(NAME dfactl_sure_scal
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/scal.sure)
    add_test(NAME dfactl_sure_swap
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/swap.sure)
    add_test(NAME dfactl_sure_copy
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/copy.sure)
    add_test(NAME dfactl_sure_rot
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/rot.sure)
    add_test(NAME dfactl_sure_iamax
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/iamax.sure)
    add_test(NAME dfactl_sure_gemv
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/gemv.sure)
    add_test(NAME dfactl_sure_gemm
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/gemm.sure
                     --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/gemm-schedule.json)
    add_test(NAME dfactl_sure_syrk
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/syrk.sure
                     --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/syrk-schedule.json)
    add_test(NAME dfactl_sure_syr2k
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/syr2k.sure
                     --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/syr2k-schedule.json)
    add_test(NAME dfactl_sure_trmm
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/trmm.sure
                     --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/trmm-schedule.json)
    add_test(NAME dfactl_sure_symm
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/symm.sure
                     --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/symm-schedule.json)
    add_test(NAME dfactl_sure_lu
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/lu.sure
                     --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/lu-schedule.json)
    add_test(NAME dfactl_sure_lu_propagate
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/lu_propagate.sure
                     --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/lu_propagate-schedule.json)
    add_test(NAME dfactl_sure_cholesky
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/cholesky.sure
                     --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/cholesky-schedule.json)
    add_test(NAME dfactl_sure_ldlt
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/ldlt.sure
                     --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/ldlt-schedule.json)
    add_test(NAME dfactl_sure_ger
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/ger.sure)
    add_test(NAME dfactl_sure_trmv
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/trmv.sure)
    add_test(NAME dfactl_sure_symv
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/symv.sure)
    add_test(NAME dfactl_sure_syr
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/syr.sure
                     --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/syr-schedule.json)
    add_test(NAME dfactl_sure_syr2
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/syr2.sure
                     --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/syr2-schedule.json)
    add_test(NAME dfactl_sure_qr_givens
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/qr_givens.sure)
    add_test(NAME dfactl_sure_qr_givens_tall
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/qr_givens_tall.sure)
    # schedule-animation export (issue #64): the emit must succeed for both schedules
    add_test(NAME dfactl_emit_matmul_linear
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/matmul.sure
                     --schedule linear --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/matmul-linear.sched.json)
    add_test(NAME dfactl_emit_matmul_free
             COMMAND ${DFA_SIMULATOR} --sure ${DOMAINFLOW_ROOT}/docs/SURE/matmul.sure
                     --schedule free --emit-schedule ${CMAKE_CURRENT_BINARY_DIR}/matmul-free.sched.json)
endif()
