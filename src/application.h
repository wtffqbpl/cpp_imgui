#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <vector>

class WindowBase {
protected:
  std::string name_;

public:
  explicit WindowBase(std::string name) : name_(std::move(name)) {}
  virtual bool render() = 0;
  ~WindowBase() = default;
};

class PopWindow : public WindowBase {
private:
  bool test_bool = false;
  int int_test = 10;
  float float_test = 5.5f;

public:
  explicit PopWindow(std::string name) : WindowBase(std::move(name)) {}

  bool render() override;
};

class Application {
private:
  GLFWwindow *window_;
  int base_win_height_;
  int base_win_width_;
  unsigned int background_texture_id_ = 0;
  std::vector<std::shared_ptr<WindowBase>> sub_windows_;


public:
  explicit Application(int height = 720, int width = 1280);
  ~Application();

  void setup_glfw();
  void setup_imgui();
  void run();
  void draw();

private:
  void read_image();

public:
  // User can inherent these functions and change these impl.
  virtual void update() {}
  virtual void setup_imgui_themes();
};
