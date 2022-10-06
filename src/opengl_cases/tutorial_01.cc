// Opening a window
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tutorial_headers.h"
#include <GLFW/glfw3.h> // include GLFW
#include <iostream>

void open_a_window() {

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    std::exit(-1);
  }

  glfwWindowHint(GLFW_SAMPLES, 4);               // 4x antialiasing
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.2
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  // To make macOS happy.
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  // don't want older opengl.
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Open a window and create its OpenGL context.
  // In the accompanying source code, this variable is global for simplicity.
  GLFWwindow *window;
  window = glfwCreateWindow(1024, 768, "Open a window", nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to open GLFW window.\n";
    glfwTerminate();
    std::exit(-1);
  }

  // Initialize GLEW
  glfwMakeContextCurrent(window);

  // Ensure we can capture the escape key being pressed below.
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
         glfwWindowShouldClose(window) == 0) {
    // Clear the screen. It's not mentioned before Tutorial 02, but it can
    // cause flickering, so it's there nonetheless.
    glClear(GL_COLOR_BUFFER_BIT);

    // Drawing nothing.

    // Swap buffers.
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}