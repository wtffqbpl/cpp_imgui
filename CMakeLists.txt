cmake_minimum_required(VERSION 3.22)
project(cxx_imgui)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(glfw3 3.4 REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

set(imgui_srcs
        third_parties/imgui/imgui.cpp
        third_parties/imgui/imgui_demo.cpp
        third_parties/imgui/imgui_draw.cpp
        third_parties/imgui/imgui_tables.cpp
        third_parties/imgui/imgui_widgets.cpp
        third_parties/imgui/backends/imgui_impl_glfw.cpp
        third_parties/imgui/backends/imgui_impl_opengl3.cpp

        third_parties/implot/implot.cpp
        third_parties/implot/implot_items.cpp
        )

set(project_srcs
        src/main.cc
        src/application.cc
        src/opengl_cases/tutorial_01.cc
        )

add_executable(cxx_imgui ${project_srcs} ${imgui_srcs})
target_compile_features(cxx_imgui PUBLIC cxx_std_20)

target_include_directories(cxx_imgui PUBLIC
        /usr/local/include
        third_parties/imgui
        third_parties/imgui/backends
        third_parties/implot

        src
        src/opengl_cases
)

target_link_libraries(cxx_imgui "-framework Cocoa")
target_link_libraries(cxx_imgui "-framework OpenGL")
target_link_libraries(cxx_imgui "-framework IOKit")
target_link_libraries(cxx_imgui glfw)
