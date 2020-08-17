
MACRO(TRACE_TEST name frame orig_img)
  
  ADD_TEST(NAME ${name} COMMAND
    ${CMAKE_CURRENT_SOURCE_DIR}/compare-trace.sh
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${name}.test_trace
    ${frame}
    ${orig_img})
ENDMACRO(TRACE_TEST)

TRACE_TEST(glxgears 238 6620)
TRACE_TEST(red_triangle 139 13732)
TRACE_TEST(textured_cube 145 14961)
TRACE_TEST(basic_shading 732 29443)
TRACE_TEST(transparency 289 19547)
TRACE_TEST(2dfonts 369 28885)
