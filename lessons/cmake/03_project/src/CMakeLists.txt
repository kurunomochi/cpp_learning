add_library(taskkit STATIC title.cpp)
add_library(TaskKit::taskkit ALIAS taskkit)
target_compile_features(taskkit PUBLIC cxx_std_20)
target_include_directories(taskkit
  PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE "${PROJECT_BINARY_DIR}/generated")
taskkit_warnings(taskkit)
