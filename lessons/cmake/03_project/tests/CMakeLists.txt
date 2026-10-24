add_executable(title_test title_test.cpp)
target_link_libraries(title_test PRIVATE TaskKit::taskkit)
taskkit_warnings(title_test)
add_test(NAME title_rules COMMAND title_test)
