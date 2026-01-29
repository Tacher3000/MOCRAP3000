cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

include(FetchContent)

FetchContent_Declare(
    Clipper2
    GIT_REPOSITORY https://github.com/AngusJohnson/Clipper2.git
    GIT_TAG        main
)

FetchContent_GetProperties(Clipper2)
if(NOT clipper2_POPULATED)
    FetchContent_Populate(Clipper2)
endif()

set(CLIPPER2_BASE_DIR "${clipper2_SOURCE_DIR}/CPP/Clipper2Lib")
set(CLIPPER2_SOURCES
    "${CLIPPER2_BASE_DIR}/src/clipper.engine.cpp"
    "${CLIPPER2_BASE_DIR}/src/clipper.offset.cpp"
    "${CLIPPER2_BASE_DIR}/src/clipper.rectclip.cpp"
)
set(CLIPPER2_INCLUDE_DIR "${CLIPPER2_BASE_DIR}/include")

add_library(CoreLib STATIC
    inputmanager.h inputmanager.cpp
    layoutstructures.h layoutstructures.cpp
    layoutoptimizer.h layoutoptimizer.cpp
    geometrynormalizer.h geometrynormalizer.cpp
    geometryutils.h geometryutils.cpp
    geometry.cpp geometry.h
    ${CLIPPER2_SOURCES}
)

target_include_directories(CoreLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CLIPPER2_INCLUDE_DIR}
)

target_link_libraries(CoreLib PRIVATE
    IoLib
    Qt6::Core
    Qt6::Gui
)
