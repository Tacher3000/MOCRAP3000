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

# --- Boost ---
FetchContent_Declare(
    Boost
    URL https://archives.boost.io/release/1.90.0/source/boost_1_90_0.tar.gz
)

FetchContent_GetProperties(Boost)
if(NOT boost_POPULATED)
    FetchContent_Populate(Boost)
endif()

# Явно задаем путь, по аналогии с Clipper2
set(BOOST_INCLUDE_DIR "${boost_SOURCE_DIR}/boost")

add_library(CoreLib STATIC
    inputmanager.h inputmanager.cpp
    layoutstructures.h layoutstructures.cpp
    layoutoptimizer.h layoutoptimizer.cpp
    geometrynormalizer.h geometrynormalizer.cpp
    geometryutils.h geometryutils.cpp
    geometry.cpp geometry.h
    ${CLIPPER2_SOURCES}
    optimizationworker.h optimizationworker.cpp
    geneticoptimizer.h geneticoptimizer.cpp
    geometryadapter.h geometryadapter.cpp
    nfpcalculator.h nfpcalculator.cpp
)

target_include_directories(CoreLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CLIPPER2_INCLUDE_DIR}
    ${boost_SOURCE_DIR}
)

target_link_libraries(CoreLib PRIVATE
    IoLib
    Qt6::Core
    Qt6::Gui
)
