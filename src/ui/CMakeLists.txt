add_library(UiLib STATIC
    MainWindow.h MainWindow.cpp

    parameterswidget.h parameterswidget.cpp

    viewerwidget.h viewerwidget.cpp
    layoutviewerwidget.h layoutviewerwidget.cpp
    geometrypainter.h geometrypainter.cpp

)

target_include_directories(UiLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(UiLib PRIVATE Qt${QT_VERSION_MAJOR}::Widgets CoreLib IoLib)
