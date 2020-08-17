
MACRO(TRACE_TEST name frame orig_img trim_img)
  
  ADD_TEST(NAME ${name} COMMAND
    ${CMAKE_CURRENT_SOURCE_DIR}/compare-trace.sh
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${name}.test_trace
    ${frame}
    ${orig_img}
    ${trim_img})
ENDMACRO(TRACE_TEST)

TRACE_TEST(glxgears 238 6620 1379)
TRACE_TEST(red_triangle 139 13732 2600)
TRACE_TEST(textured_cube 145 14961 2633)
TRACE_TEST(basic_shading 732 29443 2654)
