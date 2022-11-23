#include <glad/glad.h>
#include "application.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <fstream>
#include "program.h"
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_query.hpp>

static Program program;

static GLuint VAO = 0;
const float vertices[] = {
    -0.5f,  0.5f, -0.5f, 0.f, 0.f,   // top
    0.5f,  0.5f, -0.5f, 1.f, 0.f,
    0.5f,  0.5f,  0.5f, 1.f, 1.f,
    0.5f,  0.5f,  0.5f, 1.f, 1.f,
    -0.5f,  0.5f,  0.5f, 0.f, 1.f,
    -0.5f,  0.5f, -0.5f, 0.f, 0.f,

    -0.5f,  0.5f,  0.5f, 1.f, 1.f,   // left
    -0.5f,  0.5f, -0.5f, 1.f, 0.f,
    -0.5f, -0.5f, -0.5f, 0.f, 0.f,
    -0.5f, -0.5f, -0.5f, 0.f, 0.f,
    -0.5f, -0.5f,  0.5f, 0.f, 1.f,
    -0.5f,  0.5f,  0.5f, 1.f, 1.f,

    -0.5f, -0.5f, -0.5f, 0.f, 0.f,    // front
    0.5f, -0.5f, -0.5f, 1.f, 0.f,
    0.5f,  0.5f, -0.5f, 1.f, 1.f,
    0.5f,  0.5f, -0.5f, 1.f, 1.f,
    -0.5f,  0.5f, -0.5f, 0.f, 1.f,
    -0.5f, -0.5f, -0.5f, 0.f, 0.f,

    -0.5f, -0.5f, -0.5f, 0.f, 0.f,    // bottom
    0.5f, -0.5f, -0.5f, 1.f, 0.f,
    0.5f, -0.5f,  0.5f, 1.f, 1.f,
    0.5f, -0.5f,  0.5f, 1.f, 1.f,
    -0.5f, -0.5f,  0.5f, 0.f, 1.f,
    -0.5f, -0.5f, -0.5f, 0.f, 0.f,

    0.5f,  0.5f,  0.5f, 1.f, 1.f,    // right
    0.5f,  0.5f, -0.5f, 1.f, 0.f,
    0.5f, -0.5f, -0.5f, 0.f, 0.f,
    0.5f, -0.5f, -0.5f, 0.f, 0.f,
    0.5f, -0.5f,  0.5f, 0.f, 1.f,
    0.5f,  0.5f,  0.5f, 1.f, 1.f,

    -0.5f, -0.5f,  0.5f, 0.f, 0.f,    // back
    0.5f, -0.5f,  0.5f, 1.f, 0.f,
    0.5f,  0.5f,  0.5f, 1.f, 1.f,
    0.5f,  0.5f,  0.5f, 1.f, 1.f,
    -0.5f,  0.5f,  0.5f, 0.f, 1.f,
    -0.5f, -0.5f,  0.5f, 0.f, 0.f,
};

static void init_sphere() {
unsigned int VBO;

glGenVertexArrays(1, &VAO);
glGenBuffers(1, &VBO);

glBindVertexArray(VAO);

glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

// position attribute
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

// texcoord attribute
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
glEnableVertexAttribArray(1);

glBindVertexArray(0);


  std::string fs = "#version 330\n"
                   "precision mediump float;\n"
                   "out vec4 fragColor;\n"
                   "void main()\n"
                   "{\n"
                   "    fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
                   "}";

  std::string vs = "#version 330\n"
                   "layout (location = 0) in vec3 aPos;\n"
                   "uniform mat4 uMVP;\n"
                   "void main(void)\n"
                   "{\n"
                   "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
                   "}";

  if (!program.create(vs, fs)) {
    std::cout << "create shader fail!" << std::endl;
    return;
  }

  int posLocation = program.getAttribLocation("aPos");
  if (posLocation == -1) {
    std::cout << "not found attribute aPos!" << std::endl;
    program.release();
    return;
  }

  auto projection_ = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 10000.f);
  glm::mat4 mv(glm::identity<glm::mat4>());

  glm::mat4 MVP = projection_ * mv;
  int mvpLocation = program.getUniformLocation("uMVP");
  if (mvpLocation != -1) {
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, (float*)glm::value_ptr(MVP));
  }

}

void Application::read_image() {
  stbi_set_flip_vertically_on_load(true);
  int width, height, components;
  std::string file_name = "../imgs/galaxy_background_copy.png";
  auto bytes = stbi_load(file_name.c_str(), &width, &height, &components, 0);

  if (!background_texture_id_) {
    // TODO: in init functions.
    glGenTextures(1, &background_texture_id_);
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  {
    glBindTexture(GL_TEXTURE_2D, background_texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, bytes);
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  init_sphere();
}


static float fontSize() {
  static float size = 0.f;
  if (size == 0) {
    constexpr float base_size = 9.f;
    int x, y, width, height;
    auto monitor = glfwGetPrimaryMonitor();
    glfwGetMonitorWorkarea(monitor, &x, &y, &width, &height);
    size = width < 1024 ? base_size : width / 1024.f * base_size;
  }
  return size;
}

Application::Application(int height, int width)
    : window_(nullptr), base_win_height_(height), base_win_width_(width) {
  setup_glfw();
  setup_imgui();
}

void Application::setup_imgui_themes() {
  ImGui::StyleColorsDark();

  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowPadding = ImVec2(8, 8);
  style.WindowBorderSize = 1;
  style.WindowMenuButtonPosition = ImGuiDir_None;
  style.FrameBorderSize = 1;
  style.FrameRounding = 4;

  ImGuiIO& io = ImGui::GetIO();
  io.FontAllowUserScaling = true;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::GetIO().FontDefault = io.Fonts->AddFontFromFileTTF(
      "../font/DroidSans.ttf", fontSize(), nullptr, io.Fonts->GetGlyphRangesChineseFull());
}

void Application::setup_imgui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  // setup ImGui themes, or user can set up themes by overriding this function.
  setup_imgui_themes();

  ImGui_ImplGlfw_InitForOpenGL(window_, true);
  ImGui_ImplOpenGL3_Init();

  read_image();

  // add each windows.
  sub_windows_.emplace_back(std::make_shared<PopWindow>(PopWindow{"Test2"}));
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
  // glfwSwapInterval(1); // Enable vsync

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "failed to load glad" << std::endl;
  }

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

void Application::draw() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // render new frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  auto ui_origin = ImGui::GetCursorScreenPos();
  auto ui_size   = ImGui::GetContentRegionAvail();

#if 0
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  draw_list->AddImage(
      (ImTextureID)(background_texture_id_),
      ui_origin,
      ui_size,
      ImVec2(0.f, 1.f),
      ImVec2(1.f, 0.f));
#endif

  // draw sphere
  std::cout << ui_size.x << ", " << ui_size.y << '\n';
  glViewport(0, 0, ui_size.x, ui_size.y);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.1f, 0.2f, 0.3f, 1.0f);

  glEnable(GL_DEPTH_TEST);

  program.use();
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);

  // Render each windows.
  for (auto &sub_window : sub_windows_)
    sub_window->render();

  ImGui::Render();
}

Application::~Application() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window_);
  glfwTerminate();
}

bool PopWindow::render() {
  ImGui::SetNextWindowSizeConstraints(ImVec2(200, 200), ImVec2(800, 800));
  if (ImGui::Begin("Options", 0)) {
    // ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
    if (ImGui::BeginTable("##options", 2, ImGuiTableFlags_SizingStretchProp)) {
      static int planet_num = 1;
      static float speed = 0.1f;

      ImGui::TableNextRow(); {
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Planet Num:");

        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::PushID(&planet_num);
        if (ImGui::SliderInt("##", &planet_num, 1, 10)) {
          // TODO change planet num
        }
        ImGui::PopID();
      }

      ImGui::TableNextRow();
      {
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Speed(0.1s):");

        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::PushID(&speed);
        if (ImGui::SliderFloat("##", &speed, 0.1f, 10.f)) {
          // Change display speed
        }
        ImGui::PopID();
      }

      ImGui::EndTable();
    }
    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
    ImGui::NewLine();
  }
  ImGui::End();
}
