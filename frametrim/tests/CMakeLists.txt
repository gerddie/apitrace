
MACRO(TRACE_TEST name trace frame orig_img trim_img)
  
  ADD_TEST(NAME ${name} COMMAND
    ${CMAKE_CURRENT_SOURCE_DIR}/compare-trace.sh
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${trace}
    ${frame}
    ${orig_img}
    ${trim_img})
ENDMACRO(TRACE_TEST)

TRACE_TEST(glxgears_test glxgears.test_trace 238 6620 1379)


