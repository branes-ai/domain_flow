file (GLOB SOURCES "./*.cpp")

# TODO: automate the library test targets
#compile_all("true" "test" "dfalib/tests" "${DFA_LIBRARY}" "${SOURCES}")

add_executable(builder builder.cpp)
target_link_libraries(builder ${DFA_LIBRARY})

#add_executable(scc strongly_connected.cpp)
#target_link_libraries(scc ${DFA_LIBRARY})

#add_executable(analyze analyze.cpp)
#target_link_libraries(analyze ${DFA_LIBRARY})

#add_executable(viz visualization.cpp)
#target_link_libraries(viz ${DFA_LIBRARY})
