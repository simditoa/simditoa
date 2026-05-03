#
# Helper macro to add a C++ test
#
macro(add_cpp_test TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
  target_link_libraries(${TEST_NAME} PRIVATE simditoa)
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()
