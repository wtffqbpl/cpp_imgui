#pragma once

#include <GLFW/glfw3.h>

class Application {
private:
  GLFWwindow *window_;
  int base_win_height_;
  int base_win_width_;

public:
  explicit Application(int height = 720, int width = 1280);
  ~Application();

  void setup_glfw();
  void setup_imgui();
  void run();
  void draw();

public:
  // User can inherent these functions and change these impl.
  virtual void update() {}
  virtual void setup_imgui_themes();
  virtual void draw_impl();
};