
ADD_TEST(NAME glxgears_test   COMMAND
          ${CMAKE_CURRENT_SOURCE_DIR}/compare-trace.sh
          ${CMAKE_BINARY_DIR}
          ${CMAKE_CURRENT_SOURCE_DIR}
          glxgears.test_trace
          238
          6620
          1379)


