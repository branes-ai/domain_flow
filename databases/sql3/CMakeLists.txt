
# if you use the official SQLite install
#find_package(SQLite3 REQUIRED)
#if(SQLite3_FOUND)
#    message(STATUS "SQLite3_VERSION: ${SQLite3_VERSION}")  
#    message(STATUS "SQLite3_INCLUDE_DIRS: ${SQLite3_INCLUDE_DIRS}")  
#    message(STATUS "SQLite3_LIBRARIES: ${SQLite3_LIBRARIES}")  
#    set(SQL3_TEST sql3_test)
#    add_executable(${SQL3_TEST} sql3_test.cpp)
#    target_include_directories(${SQL3_TEST} PRIVATE ${SQLite3_INCLUDE_DIRS})
#    target_link_libraries(${SQL3_TEST} PRIVATE "SQLite3::SQLite3")
#    set_target_properties(${SQL3_TEST} PROPERTIES FOLDER "dfa/database")
#endif()

# if you use the vcpkg install
find_package(unofficial-sqlite3 CONFIG REQUIRED)
if(unofficial-sqlite3_FOUND)
    
    set(SQL3_TEST sql3_test)
    add_executable(${SQL3_TEST} sql3_test.cpp)
    target_include_directories(${SQL3_TEST} PRIVATE ${SQLite3_INCLUDE_DIRS})
    target_link_libraries(${SQL3_TEST} PRIVATE unofficial::sqlite3::sqlite3)
    set_target_properties(${SQL3_TEST} PROPERTIES FOLDER "dfa/database")
else()
    message(STATUS "SQLite3 not found")
endif()
