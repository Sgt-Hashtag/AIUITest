cmake_minimum_required(VERSION 3.16)
project(AiShapeDemo LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(AiShapeDemo
    main.cpp
    ShapeWindow.cpp ShapeWindow.h
    AiController.cpp AiController.h
)

target_link_libraries(AiShapeDemo PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)