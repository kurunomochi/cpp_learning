cmake_minimum_required(VERSION 3.24)
project(TaskKit VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(TASKKIT_BUILD_APP "Build the demo executable" ON)
include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/Warnings.cmake)

configure_file(cmake/config.hpp.in generated/taskkit/config.hpp @ONLY)
add_subdirectory(src)
if(TASKKIT_BUILD_APP)
  add_subdirectory(app)
endif()
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(TARGETS taskkit EXPORT TaskKitTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
if(TASKKIT_BUILD_APP)
  install(TARGETS taskkit_app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT TaskKitTargets NAMESPACE TaskKit::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TaskKit)
configure_package_config_file(cmake/TaskKitConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/TaskKitConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TaskKit)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/TaskKitConfigVersion.cmake"
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/TaskKitConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/TaskKitConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TaskKit)
