cmake_minimum_required(VERSION 3.16)

project(MOCRAP3000 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/wd4996)
endif()

include(FetchContent)
FetchContent_Declare(
    libdxfrw
    GIT_REPOSITORY https://github.com/qgis/libdxfrw.git
    GIT_TAG master
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  ${CMAKE_SOURCE_DIR}/cmake/libdxfrw_patch/CMakeLists.txt
                  ${CMAKE_BINARY_DIR}/_deps/libdxfrw-src/CMakeLists.txt
)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(libdxfrw)

add_subdirectory(src)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MOCRAP3000
        MANUAL_FINALIZATION
        src/main.cpp
        resources/resources.qrc
    )
else()
    if(ANDROID)
        add_library(MOCRAP3000 SHARED src/main.cpp)
    else()
        add_executable(MOCRAP3000 src/main.cpp)
    endif()
endif()

target_link_libraries(MOCRAP3000 PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    UiLib
    CoreLib
    IoLib
)

set_target_properties(MOCRAP3000 PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.MOCRAP3000
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS MOCRAP3000
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MOCRAP3000)
endif()
