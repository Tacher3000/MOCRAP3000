add_library(CoreLib STATIC

    inputmanager.h inputmanager.cpp
    layoutstructures.h layoutstructures.cpp
    layoutoptimizer.h layoutoptimizer.cpp
    geometrynormalizer.h geometrynormalizer.cpp
    geometryutils.h geometryutils.cpp

)

target_include_directories(CoreLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CoreLib PRIVATE IoLib Qt6::Core Qt6::Gui)
