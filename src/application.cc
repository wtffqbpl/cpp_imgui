#include "application.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>

Application::Application(int height, int width)
    : window_(nullptr), base_win_height_(height), base_win_width_(width) {
  setup_glfw();
  setup_imgui();
}

void Application::setup_imgui_themes() { ImGui::StyleColorsDark(); }

void Application::setup_imgui() {
  const char *glsl_version = "#version 150";
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  // setup ImGui themes, or user can set up themes by overriding this function.
  setup_imgui_themes();

  ImGui_ImplGlfw_InitForOpenGL(window_, true);
  ImGui_ImplOpenGL3_Init(glsl_version);
}

void Application::setup_glfw() {
  glfwSetErrorCallback([](int error, const char *desc) {
    std::cerr << "Failed. error_code = " << error << ", msg = " << desc << '\n';
  });

  // check that glfw initialized properly.
  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac

  // Initialize the new window.
  window_ = glfwCreateWindow(base_win_width_, base_win_height_, "Chat root",
                             nullptr, nullptr);

  // check to see if the window initialized properly.
  if (window_ == nullptr) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  // Make the new window the current context.
  glfwMakeContextCurrent(window_);
  glfwSwapInterval(1); // Enable vsync

  // Setting the background to white.
  glClearColor(255, 255, 255, 255);
}

void Application::run() {
  while (!glfwWindowShouldClose(window_)) {

    // check for if the user process escape, if so glfwSetWindowShouldClose ==
    // true
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window_, true);

    // Getting the width and height for the view port.
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    glViewport(0, 0, width, height);

    // call the update and draw functions.
    update();
    draw();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // swap the buffers and Poll events.
    glfwSwapBuffers(window_);
    glfwPollEvents();
  }
}

void Application::draw_impl() {
  static bool test_bool = false;
  static int int_test = 10;
  static float float_test = 5.5f;

  ImGui::SetNextWindowSize(ImVec2(100, 100));
  if (ImGui::Begin("Test2", nullptr, ImGuiWindowFlags_NoResize)) {
    ImGui::Text("Draw a window.");
  }
  ImGui::End();

  ImGui::SetNextWindowSize(ImVec2(500, 500));
  if (ImGui::Begin("Test", nullptr, ImGuiWindowFlags_NoResize)) {
    ImGui::Checkbox("Test Bool", &test_bool);
    if (ImGui::Button("Click me!"))
      test_bool ^= true;

    ImGui::Text("Int");
    ImGui::SliderInt("##d", &int_test, 1, 25);
    ImGui::Text("Float");
    ImGui::SliderFloat("##dd", &float_test, 0.1f, 15.5f);

    // ImGui::SetCursorPos(ImVec2(400, 100));
    if (ImGui::Button("Test", ImVec2(50, 50))) {
    }
  }
  ImGui::End();

  if (test_bool || int_test >= 15 || float_test > 10.f) {
    ImGui::SetNextWindowSize(ImVec2(300, 300));
    if (ImGui::Begin("Window2")) {
      ImGui::Text("Hello, this is another window!");
    }
    ImGui::End();
  }
}

void Application::draw() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // render new frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  draw_impl();
  ImGui::Render();
}

Application::~Application() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window_);
  glfwTerminate();
}