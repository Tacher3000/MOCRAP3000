add_library(CoreLib STATIC
    geometry.h geometry.cpp
    inputmanager.h inputmanager.cpp

)

target_include_directories(CoreLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CoreLib PRIVATE IoLib)
