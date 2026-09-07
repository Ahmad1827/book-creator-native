cmake_minimum_required(VERSION 3.20)
project(book-creator-native VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg)

include_directories(
    src
    src/models
    src/canvas
    src/ui
    src/utils
)

add_executable(book-creator-native
    src/main.cpp
    src/models/types.h
    src/models/constants.h
    src/canvas/theme_decors.h
    src/canvas/theme_decors.cpp
    src/canvas/book_canvas_view.h
    src/canvas/book_canvas_view.cpp
    src/ui/drawing_toolbox_dock.h
    src/ui/drawing_toolbox_dock.cpp
    src/ui/layers_dock.h
    src/ui/layers_dock.cpp
    src/ui/main_window.h
    src/ui/main_window.cpp
    src/utils/pdf_exporter.h
    src/utils/pdf_exporter.cpp
)

target_link_libraries(book-creator-native PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
)