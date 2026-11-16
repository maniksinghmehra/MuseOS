cmake_minimum_required(VERSION 3.21)

project(MuseOSFrontend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

add_executable(MuseOSFrontend
    src/gui_main.cpp
)

target_link_libraries(MuseOSFrontend PRIVATE
    Qt6::Widgets
    Qt6::Sql
)

set_target_properties(MuseOSFrontend PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)