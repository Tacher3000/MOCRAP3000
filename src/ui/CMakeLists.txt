add_library(UiLib STATIC
    MainWindow.h MainWindow.cpp
)

target_include_directories(UiLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(UiLib PRIVATE Qt${QT_VERSION_MAJOR}::Widgets CoreLib IoLib)