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
    src/models/book_format.h
    src/canvas/theme_decors.h
    src/canvas/theme_decors.cpp
    src/canvas/canvas_items.h
    src/canvas/canvas_items.cpp
    src/canvas/book_canvas_view.h
    src/canvas/book_canvas_view.cpp
    src/ui/drawing_toolbox_dock.h
    src/ui/drawing_toolbox_dock.cpp
    src/ui/layers_dock.h
    src/ui/layers_dock.cpp
    src/ui/new_book_dialog.h
    src/ui/new_book_dialog.cpp
    src/ui/bookshelf_view.h
    src/ui/bookshelf_view.cpp
    src/ui/main_window.h
    src/ui/main_window.cpp
    src/utils/pdf_exporter.h
    src/utils/pdf_exporter.cpp
    src/utils/spread_renderer.h
    src/utils/spread_renderer.cpp
    src/utils/asset_store.h
    src/utils/asset_store.cpp
    src/ui/page_sorter_dialog.h
    src/ui/page_sorter_dialog.cpp
    src/ui/sketch.h
    src/ui/sketch.cpp
    src/ui/export_dialog.h
    src/ui/export_dialog.cpp
    src/ui/settings_dialog.h
    src/ui/settings_dialog.cpp
    src/ui/story_helper_dialog.h
    src/ui/story_helper_dialog.cpp
    src/utils/app_settings.h
    src/utils/app_settings.cpp
    src/utils/story_helper.h
    src/utils/story_helper.cpp
    assets/resources.qrc
)

target_link_libraries(book-creator-native PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
)
if(WIN32)
    # A normal windowed app (no console), named for the people using it.
    set_target_properties(book-creator-native PROPERTIES
        WIN32_EXECUTABLE ON
        OUTPUT_NAME "StorybookStudio"
    )
endif()

if(MSVC)
    # Sources contain non-ASCII text such as "…".
    target_compile_options(book-creator-native PRIVATE /utf-8)
endif()
