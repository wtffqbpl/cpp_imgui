cmake_minimum_required(VERSION 3.22)
project(cxx_imgui)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_BINDIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_INSTALL_LIBDIR ${CMAKE_SOURCE_DIR}/lib)

# find_package(SDL2 REQUIRED)
# include_directories(${SDL2_INCLUDE_DIRS})

find_package(glfw3 3.4 REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})

find_package(GLUT REQUIRED)

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
        # src/application.cc
        # src/main.cc
        # src/program.cpp
        src/nbody/nbody.cpp
        src/nbody/render_particles.cpp
        # src/external/glad/glad.c
        )

set(target cpp_imgui)
add_executable(${target} ${imgui_srcs})
target_compile_features(${target} PUBLIC cxx_std_20)
install(TARGETS ${target})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/imgs
        DESTINATION ${CMAKE_SOURCE_DIR}/bin/)

target_include_directories(${target} PUBLIC
        /usr/local/include
        third_parties/imgui
        third_parties/imgui/backends
        third_parties/implot
        third_parties
        src
        src/external
        src/nbody
        inc
)

target_link_libraries(${target} PRIVATE glfw)
target_link_libraries(${target} PRIVATE "-framework GLUT")
target_link_libraries(${target} PRIVATE "-framework OpenGL")