file(GLOB LIBDXFRW_SOURCES
    ${libdxfrw_SOURCE_DIR}/src/*.cpp
    ${libdxfrw_SOURCE_DIR}/src/intern/*.cpp
)

add_library(IoLib STATIC
    dxfreader.h dxfreader.cpp
    ${LIBDXFRW_SOURCES}
)

target_include_directories(IoLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${libdxfrw_SOURCE_DIR}/src
    ${libdxfrw_SOURCE_DIR}/src/intern
)

target_link_libraries(IoLib PRIVATE
    Qt6::Core
)

target_compile_features(IoLib PRIVATE cxx_std_20)
set_target_properties(IoLib PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
