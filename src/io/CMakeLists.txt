add_library(IoLib STATIC




)

target_include_directories(IoLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${libdxfrw_SOURCE_DIR}/src
)

target_link_libraries(IoLib PRIVATE
    Qt6::Core
    dxfrw
)

target_compile_features(IoLib PRIVATE cxx_std_20)
set_target_properties(IoLib PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
