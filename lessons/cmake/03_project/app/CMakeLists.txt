add_executable(taskkit_app main.cpp)
target_link_libraries(taskkit_app PRIVATE TaskKit::taskkit)
taskkit_warnings(taskkit_app)
