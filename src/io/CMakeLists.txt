add_library(IoLib STATIC

)

target_include_directories(IoLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${libdxfrw_SOURCE_DIR}/src)
target_link_libraries(IoLib PRIVATE dxfrw) 