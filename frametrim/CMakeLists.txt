include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/cli
    ${CMAKE_SOURCE_DIR}/lib/highlight
    ${CMAKE_SOURCE_DIR}/helpers
    ${CMAKE_SOURCE_DIR}/retrace
    ${CMAKE_BINARY_DIR}/dispatch
    ${CMAKE_SOURCE_DIR}/dispatch
    ${CMAKE_SOURCE_DIR}/lib/image
    ${CMAKE_SOURCE_DIR}/lib/ubjson
    ${CMAKE_SOURCE_DIR}/thirdparty/dxerr
    ${CMAKE_SOURCE_DIR}/thirdparty/mhook/mhook-lib
)

cmake_policy(SET CMP0079 NEW)

add_executable(apitrace-frametrim
   ftb_dependecyobject.cpp
   ftb_frametrimmer.cpp
   ftb_main.cpp
   ft_matrixstate.cpp
   ft_tracecall.cpp)

set_target_properties(apitrace-frametrim PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

target_link_libraries (apitrace-frametrim
    retrace_common
    glretrace_common
    )

add_subdirectory (tests)
