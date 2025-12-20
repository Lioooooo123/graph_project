/**
 * @file main.cpp
 * @author Ross Ning (rossning92@gmail.com)
 * @brief Real-time black hole rendering in OpenGL.
 * @version 0.1
 * @date 2020-08-29
 *
 * @copyright Copyright (c) 2020
 *
 */

#include <algorithm>
#include <assert.h>
#include <filesystem>
#include <map>
#include <stdio.h>
#include <vector>

#include <cstddef>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <imgui.h>

#include "GLDebugMessageCallback.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "render.h"
#include "shader.h"
#include "texture.h"

static int SCR_WIDTH = 1920;
static int SCR_HEIGHT = 1080;

static float mouseX, mouseY;

static const bool kEnableImGui = false;
static const int kMaxBloomIter = 5;
static const float kRenderScale = 0.75f;  // Render at 75% resolution for performance

#define IMGUI_TOGGLE(NAME, DEFAULT)                                            \
  static bool NAME = DEFAULT;                                                  \
  if (kEnableImGui) {                                                          \
    ImGui::Checkbox(#NAME, &NAME);                                             \
  }                                                                            \
  rtti.floatUniforms[#NAME] = NAME ? 1.0f : 0.0f;

#define IMGUI_SLIDER(NAME, DEFAULT, MIN, MAX)                                  \
  static float NAME = DEFAULT;                                                 \
  if (kEnableImGui) {                                                          \
    ImGui::SliderFloat(#NAME, &NAME, MIN, MAX);                                \
  }                                                                            \
  rtti.floatUniforms[#NAME] = NAME;

static void glfwErrorCallback(int error, const char *description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

struct Mesh {
  GLuint vao = 0;
  GLsizei vertexCount = 0;
};

// 缓动函数：平滑的缓入缓出效果
float easeInOutCubic(float t) {
  return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

float easeInOutQuint(float t) {
  return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 5.0f) / 2.0f;
}

// 更平滑的缓动：结合 sine 和 cubic
float easeInOutSine(float t) {
  return -(cosf(3.14159265f * t) - 1.0f) / 2.0f;
}

glm::vec3 calculateBezierPoint(float t, const glm::vec3 &p0,
                               const glm::vec3 &p1, const glm::vec3 &p2,
                               const glm::vec3 &p3) {
  float u = 1.0f - t;
  float tt = t * t;
  float uu = u * u;
  float uuu = uu * u;
  float ttt = tt * t;

  return uuu * p0 + 3.0f * uu * t * p1 + 3.0f * u * tt * p2 + ttt * p3;
}

// 计算贝塞尔曲线在某点的切线方向（用于相机朝向）
glm::vec3 calculateBezierTangent(float t, const glm::vec3 &p0,
                                 const glm::vec3 &p1, const glm::vec3 &p2,
                                 const glm::vec3 &p3) {
  float u = 1.0f - t;
  float uu = u * u;
  float tt = t * t;
  
  glm::vec3 tangent = -3.0f * uu * p0 + 3.0f * uu * p1 - 6.0f * u * t * p1 
                      - 3.0f * tt * p2 + 6.0f * u * t * p2 + 3.0f * tt * p3;
  return glm::normalize(tangent);
}

Mesh createSatelliteMesh() {
  struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
  };

  std::vector<Vertex> vertices;

  auto addFace = [&](const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c,
                     const glm::vec3 &d) {
    glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
    vertices.push_back({a, n});
    vertices.push_back({b, n});
    vertices.push_back({c, n});
    vertices.push_back({a, n});
    vertices.push_back({c, n});
    vertices.push_back({d, n});
  };

  auto addTri = [&](const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) {
    glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
    vertices.push_back({a, n});
    vertices.push_back({b, n});
    vertices.push_back({c, n});
  };

  auto addBox = [&](const glm::vec3 &center, const glm::vec3 &halfSize) {
    glm::vec3 p000 = center + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z);
    glm::vec3 p001 = center + glm::vec3(-halfSize.x, -halfSize.y, halfSize.z);
    glm::vec3 p010 = center + glm::vec3(-halfSize.x, halfSize.y, -halfSize.z);
    glm::vec3 p011 = center + glm::vec3(-halfSize.x, halfSize.y, halfSize.z);
    glm::vec3 p100 = center + glm::vec3(halfSize.x, -halfSize.y, -halfSize.z);
    glm::vec3 p101 = center + glm::vec3(halfSize.x, -halfSize.y, halfSize.z);
    glm::vec3 p110 = center + glm::vec3(halfSize.x, halfSize.y, -halfSize.z);
    glm::vec3 p111 = center + glm::vec3(halfSize.x, halfSize.y, halfSize.z);

    // +X, -X, +Y, -Y, +Z, -Z faces
    addFace(p100, p110, p111, p101);
    addFace(p010, p000, p001, p011);
    addFace(p110, p010, p011, p111);
    addFace(p000, p100, p101, p001);
    addFace(p101, p111, p011, p001);
    addFace(p100, p000, p010, p110);
  };

  // Helper to add a cylinder along Y axis
  auto addCylinder = [&](const glm::vec3 &base, float radius, float height, int segments) {
    for (int i = 0; i < segments; i++) {
      float a0 = 2.0f * 3.14159f * i / segments;
      float a1 = 2.0f * 3.14159f * (i + 1) / segments;
      glm::vec3 p0 = base + glm::vec3(cos(a0) * radius, 0, sin(a0) * radius);
      glm::vec3 p1 = base + glm::vec3(cos(a1) * radius, 0, sin(a1) * radius);
      glm::vec3 p2 = p1 + glm::vec3(0, height, 0);
      glm::vec3 p3 = p0 + glm::vec3(0, height, 0);
      addFace(p0, p1, p2, p3);
      // Top cap
      glm::vec3 topCenter = base + glm::vec3(0, height, 0);
      addTri(topCenter, p3, p2);
      // Bottom cap
      addTri(base, p1, p0);
    }
  };

  // Helper to add a cone along Y axis
  auto addCone = [&](const glm::vec3 &base, float radius, float height, int segments) {
    glm::vec3 tip = base + glm::vec3(0, height, 0);
    for (int i = 0; i < segments; i++) {
      float a0 = 2.0f * 3.14159f * i / segments;
      float a1 = 2.0f * 3.14159f * (i + 1) / segments;
      glm::vec3 p0 = base + glm::vec3(cos(a0) * radius, 0, sin(a0) * radius);
      glm::vec3 p1 = base + glm::vec3(cos(a1) * radius, 0, sin(a1) * radius);
      addTri(p0, p1, tip);
      addTri(base, p1, p0);
    }
  };

  // ========== MAIN BODY ==========
  // Central octagonal body (more interesting than a box)
  float bodyRadius = 0.32f;
  float bodyHeight = 0.5f;
  int bodySides = 8;
  glm::vec3 bodyBase = glm::vec3(0, -bodyHeight / 2, 0);
  for (int i = 0; i < bodySides; i++) {
    float a0 = 2.0f * 3.14159f * i / bodySides;
    float a1 = 2.0f * 3.14159f * (i + 1) / bodySides;
    glm::vec3 p0 = bodyBase + glm::vec3(cos(a0) * bodyRadius, 0, sin(a0) * bodyRadius);
    glm::vec3 p1 = bodyBase + glm::vec3(cos(a1) * bodyRadius, 0, sin(a1) * bodyRadius);
    glm::vec3 p2 = p1 + glm::vec3(0, bodyHeight, 0);
    glm::vec3 p3 = p0 + glm::vec3(0, bodyHeight, 0);
    addFace(p0, p1, p2, p3);
    // Top cap
    glm::vec3 topCenter = glm::vec3(0, bodyHeight / 2, 0);
    addTri(topCenter, p3, p2);
    // Bottom cap
    glm::vec3 bottomCenter = glm::vec3(0, -bodyHeight / 2, 0);
    addTri(bottomCenter, p1, p0);
  }

  // ========== SOLAR PANEL ARMS ==========
  // Connection arms from body to panels
  addBox(glm::vec3(-0.5f, 0.0f, 0.0f), glm::vec3(0.18f, 0.04f, 0.04f));
  addBox(glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(0.18f, 0.04f, 0.04f));

  // ========== SOLAR PANELS (segmented for realism) ==========
  // Left panel - main frame
  float panelX = -1.15f;
  float panelWidth = 0.75f;
  float panelHeight = 0.45f;
  addBox(glm::vec3(panelX, 0.0f, 0.0f), glm::vec3(panelWidth, 0.02f, panelHeight));
  // Panel frame edges
  addBox(glm::vec3(panelX, 0.025f, panelHeight - 0.02f), glm::vec3(panelWidth, 0.015f, 0.02f));
  addBox(glm::vec3(panelX, 0.025f, -panelHeight + 0.02f), glm::vec3(panelWidth, 0.015f, 0.02f));
  addBox(glm::vec3(panelX - panelWidth + 0.02f, 0.025f, 0.0f), glm::vec3(0.02f, 0.015f, panelHeight - 0.02f));
  addBox(glm::vec3(panelX + panelWidth - 0.02f, 0.025f, 0.0f), glm::vec3(0.02f, 0.015f, panelHeight - 0.02f));
  // Panel grid lines
  for (int i = 1; i < 4; i++) {
    float offset = panelX - panelWidth + (2.0f * panelWidth * i / 4.0f);
    addBox(glm::vec3(offset, 0.022f, 0.0f), glm::vec3(0.008f, 0.008f, panelHeight - 0.03f));
  }

  // Right panel - main frame
  panelX = 1.15f;
  addBox(glm::vec3(panelX, 0.0f, 0.0f), glm::vec3(panelWidth, 0.02f, panelHeight));
  // Panel frame edges
  addBox(glm::vec3(panelX, 0.025f, panelHeight - 0.02f), glm::vec3(panelWidth, 0.015f, 0.02f));
  addBox(glm::vec3(panelX, 0.025f, -panelHeight + 0.02f), glm::vec3(panelWidth, 0.015f, 0.02f));
  addBox(glm::vec3(panelX - panelWidth + 0.02f, 0.025f, 0.0f), glm::vec3(0.02f, 0.015f, panelHeight - 0.02f));
  addBox(glm::vec3(panelX + panelWidth - 0.02f, 0.025f, 0.0f), glm::vec3(0.02f, 0.015f, panelHeight - 0.02f));
  // Panel grid lines
  for (int i = 1; i < 4; i++) {
    float offset = panelX - panelWidth + (2.0f * panelWidth * i / 4.0f);
    addBox(glm::vec3(offset, 0.022f, 0.0f), glm::vec3(0.008f, 0.008f, panelHeight - 0.03f));
  }

  // ========== ANTENNA DISH ==========
  // Dish base/mount on top
  addCylinder(glm::vec3(0, 0.25f, 0), 0.08f, 0.06f, 12);
  // Dish arm
  addBox(glm::vec3(0, 0.38f, 0.12f), glm::vec3(0.02f, 0.08f, 0.02f));
  // Simplified dish (cone shape)
  addCone(glm::vec3(0, 0.32f, 0.22f), 0.12f, 0.08f, 12);

  // ========== COMMUNICATION ANTENNAS ==========
  // Small antenna masts
  addCylinder(glm::vec3(0.15f, 0.25f, -0.15f), 0.015f, 0.25f, 6);
  addCylinder(glm::vec3(-0.15f, 0.25f, 0.15f), 0.015f, 0.2f, 6);
  // Antenna tips
  addBox(glm::vec3(0.15f, 0.52f, -0.15f), glm::vec3(0.025f, 0.025f, 0.025f));
  addBox(glm::vec3(-0.15f, 0.47f, 0.15f), glm::vec3(0.02f, 0.02f, 0.02f));

  // ========== THRUSTERS ==========
  // Bottom thrusters (4 corner nozzles)
  float thrusterOffset = 0.2f;
  addCone(glm::vec3(thrusterOffset, -0.25f, thrusterOffset), 0.04f, -0.08f, 8);
  addCone(glm::vec3(-thrusterOffset, -0.25f, thrusterOffset), 0.04f, -0.08f, 8);
  addCone(glm::vec3(thrusterOffset, -0.25f, -thrusterOffset), 0.04f, -0.08f, 8);
  addCone(glm::vec3(-thrusterOffset, -0.25f, -thrusterOffset), 0.04f, -0.08f, 8);

  // ========== SENSOR EQUIPMENT ==========
  // Front sensor package
  addBox(glm::vec3(0, 0, 0.38f), glm::vec3(0.12f, 0.1f, 0.06f));
  addCylinder(glm::vec3(0.06f, -0.02f, 0.44f), 0.025f, 0.04f, 8);
  addCylinder(glm::vec3(-0.06f, -0.02f, 0.44f), 0.025f, 0.04f, 8);

  // ========== DECORATIVE DETAILS ==========
  // Body accent strips
  for (int i = 0; i < 4; i++) {
    float angle = 3.14159f / 4.0f + i * 3.14159f / 2.0f;
    glm::vec3 stripPos = glm::vec3(cos(angle) * 0.33f, 0, sin(angle) * 0.33f);
    addBox(stripPos, glm::vec3(0.015f, 0.26f, 0.015f));
  }

  GLuint vao, vbo;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
               vertices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, normal));

  glBindVertexArray(0);

  Mesh mesh;
  mesh.vao = vao;
  mesh.vertexCount = (GLsizei)vertices.size();
  return mesh;
}

struct CameraState {
  glm::vec3 pos;
  glm::vec3 target;
  float fovScale;
  float rollRadians;
  glm::mat4 view;
  glm::mat4 projection;
};

CameraState computeCameraState(double timeSeconds, int width, int height,
                               float mouseX, float mouseY,
                               bool mouseControlEnabled, bool frontView,
                               bool topView, float cameraRollDeg,
                               float fovScale, bool autopilotActive,
                               const glm::vec3 &autopilotPos) {
  CameraState cs{};
  cs.fovScale = fovScale;
  cs.rollRadians = glm::radians(cameraRollDeg);

  if (autopilotActive) {
    cs.pos = autopilotPos;
  } else if (mouseControlEnabled) {
    glm::vec2 mouse =
        glm::clamp(glm::vec2(mouseX, mouseY) / glm::vec2(width, height),
                   glm::vec2(0.0f), glm::vec2(1.0f)) -
        glm::vec2(0.5f);
    cs.pos = glm::vec3(-cos(mouse.x * 10.0f) * 15.0f, mouse.y * 30.0f,
                       sin(mouse.x * 10.0f) * 15.0f);
  } else if (frontView) {
    cs.pos = glm::vec3(10.0f, 1.0f, 10.0f);
  } else if (topView) {
    cs.pos = glm::vec3(15.0f, 15.0f, 0.0f);
  } else {
    cs.pos = glm::vec3(-cos((float)timeSeconds * 0.1f) * 15.0f,
                       sin((float)timeSeconds * 0.1f) * 15.0f,
                       sin((float)timeSeconds * 0.1f) * 15.0f);
  }

  cs.target = glm::vec3(0.0f);

  float aspect = (float)width / (float)height;
  float fovY = 2.0f * atan(0.5f * fovScale);
  glm::vec3 up =
      glm::normalize(glm::vec3(sin(cs.rollRadians), cos(cs.rollRadians), 0.0f));

  cs.view = glm::lookAt(cs.pos, cs.target, up);
  cs.projection = glm::perspective(fovY, aspect, 0.1f, 500.0f);

  return cs;
}

glm::mat4 computeSatelliteModel(double timeSeconds, const glm::vec3 &worldPos) {
  const float selfSpinSpeed = 0.8f;

  glm::mat4 model = glm::translate(glm::mat4(1.0f), worldPos);
  model = glm::rotate(model, glm::radians(-10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
  model = glm::rotate(model, (float)(timeSeconds * selfSpinSpeed),
                      glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::scale(model, glm::vec3(0.18f));
  return model;
}

void renderSatellite(const Mesh &mesh, GLuint program, const glm::mat4 &model,
                     const glm::mat4 &view, const glm::mat4 &projection,
                     const glm::vec3 &cameraPos, const glm::vec3 &lightDir) {
  glUseProgram(program);

  glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE,
                     glm::value_ptr(model));
  glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE,
                     glm::value_ptr(view));
  glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE,
                     glm::value_ptr(projection));
  glUniform3fv(glGetUniformLocation(program, "viewPos"), 1,
               glm::value_ptr(cameraPos));
  glUniform3fv(glGetUniformLocation(program, "lightDir"), 1,
               glm::value_ptr(lightDir));
  glUniform3f(glGetUniformLocation(program, "lightColor"), 1.0f, 0.95f, 0.85f);
  glUniform3f(glGetUniformLocation(program, "rimColor"), 1.4f, 1.2f, 0.95f);
  glUniform1f(glGetUniformLocation(program, "rimStrength"), 1.35f);

  glBindVertexArray(mesh.vao);
  glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
  glBindVertexArray(0);

  glUseProgram(0);
}

void mouseCallback(GLFWwindow * /*window*/, double x, double y) {
  mouseX = (float)x;
  mouseY = (float)y;
}

class PostProcessPass {
private:
  GLuint program;

public:
  PostProcessPass(const std::string &fragShader) {
    this->program = createShaderProgram("shader/simple.vert", fragShader);

    glUseProgram(this->program);
    glUniform1i(glGetUniformLocation(program, "texture0"), 0);
    glUseProgram(0);
  }

  void render(GLuint inputColorTexture, int width, int height,
              GLuint destFramebuffer = 0) {
    glBindFramebuffer(GL_FRAMEBUFFER, destFramebuffer);

    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(this->program);

    glUniform2f(glGetUniformLocation(this->program, "resolution"), (float)width,
                (float)height);

    glUniform1f(glGetUniformLocation(this->program, "time"),
                (float)glfwGetTime());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputColorTexture);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glUseProgram(0);
  }
};

int main(int argc, char **argv) {
  // Ensure working directory is where the executable lives so relative asset
  // paths (assets/, shader/) are found even when launched from Finder.
  try {
    std::filesystem::path exePath =
        std::filesystem::absolute(argv[0]).parent_path();
    std::filesystem::current_path(exePath);
  } catch (...) {
  }

  // Setup window
  glfwSetErrorCallback(glfwErrorCallback);
  if (!glfwInit())
    return 1;

  // Select a modern core profile before creating the window so GLEW/GL3 loader
  // can fetch symbols.
#if __APPLE__
  // macOS supports up to 4.1 core; ask for 4.1 so GLSL 330 shaders are
  // accepted.
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
  // GL 3.0 + GLSL 130
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

  // Create window with graphics context (windowed)
  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Wormhole", NULL, NULL);
  if (window == NULL)
    return 1;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync
  glfwSetCursorPosCallback(window, mouseCallback);
  glfwSetWindowPos(window, 0, 0);

  // GLEW needs experimental on core profile to load all symbols on macOS.
  glewExperimental = GL_TRUE;
  bool err = glewInit() != GLEW_OK;
  if (err) {
    fprintf(stderr, "Failed to initialize OpenGL loader!\n");
    return 1;
  }

  if (0) {
    // Enable the debugging layer of OpenGL
    //
    // GL_DEBUG_OUTPUT - Faster version but not useful for breakpoints
    // GL_DEBUG_OUTPUT_SYNCHRONUS - Callback is in sync with errors, so a
    // breakpoint can be placed on the callback in order to get a stacktrace for
    // the GL error. (enable together with GL_DEBUG_OUTPUT !)

    glEnable(GL_DEBUG_OUTPUT);
    // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    // Set the function that will be triggered by the callback, the second
    // parameter is the data parameter of the callback, it can be useful for
    // different contexts but isn't necessary for our simple use case.
    glDebugMessageCallback(GLDebugMessageCallback, nullptr);
  }

  if (kEnableImGui) {
    // Decide GL+GLSL versions (must match hints used above)
#if __APPLE__
    const char *glsl_version = "#version 330 core";
#else
    const char *glsl_version = "#version 130";
#endif

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
  }

  GLuint fboBlackhole = 0, texBlackhole = 0;

  GLuint quadVAO = createQuadVAO();
  glBindVertexArray(quadVAO);

  Mesh satelliteMesh = createSatelliteMesh();
  GLuint satelliteProgram =
      createShaderProgram("shader/satellite.vert", "shader/satellite.frag");
  GLuint blackholeProgram =
      createShaderProgram("shader/simple.vert", "shader/blackhole_main.frag");

  // Main loop
  PostProcessPass passthrough("shader/passthrough.frag");

  double lastFrameTime = glfwGetTime();
  bool autopilotActive = false;
  double autopilotT = 0.0;
  bool prevCKey = false;
  const float autopilotDuration = 18.0f;  // 增加动画时长让动画更从容
  
  // 优化的贝塞尔曲线控制点 - 创建更优美的螺旋接近路径
  const glm::vec3 bezierP0 = glm::vec3(25.0f, 12.0f, 25.0f);   // 起点：远处高位
  const glm::vec3 bezierP1 = glm::vec3(-15.0f, 8.0f, 20.0f);   // 控制点1：绕到左侧
  const glm::vec3 bezierP2 = glm::vec3(12.0f, 3.0f, 8.0f);     // 控制点2：绕到右侧低位
  const glm::vec3 bezierP3 = glm::vec3(0.0f, 1.0f, 5.0f);      // 终点：靠近黑洞

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    double now = glfwGetTime();
    double deltaTime = now - lastFrameTime;
    lastFrameTime = now;

    bool cPressed = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
    if (cPressed && !prevCKey) {
      autopilotActive = !autopilotActive;
      if (autopilotActive)
        autopilotT = 0.0;
    }
    prevCKey = cPressed;

    if (autopilotActive) {
      autopilotT = std::min(1.0, autopilotT + deltaTime / autopilotDuration);
    }

    if (kEnableImGui) {
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
    }

    // ImGui::ShowDemoWindow();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0) {
      glfwSwapBuffers(window);
      continue;
    }
    glViewport(0, 0, width, height);

    // renderScene(fboBlackhole);

    static GLuint galaxy = loadCubemap("assets/skybox_nebula_dark");
    static GLuint colorMap = loadTexture2D("assets/color_map.png");

    static int renderWidth = 0;
    static int renderHeight = 0;
    static GLuint texBrightness = 0;
    static GLuint texDownsampled[kMaxBloomIter] = {};
    static GLuint texUpsampled[kMaxBloomIter] = {};
    static GLuint texBloomFinal = 0;
    static GLuint texTonemapped = 0;
    
    // Use scaled resolution for expensive ray marching pass
    int scaledWidth = (int)(width * kRenderScale);
    int scaledHeight = (int)(height * kRenderScale);
    if (scaledWidth < 1) scaledWidth = 1;
    if (scaledHeight < 1) scaledHeight = 1;
    
    if (scaledWidth != renderWidth || scaledHeight != renderHeight || texBlackhole == 0) {
      renderWidth = scaledWidth;
      renderHeight = scaledHeight;
      SCR_WIDTH = width;
      SCR_HEIGHT = height;

      texBlackhole = createColorTexture(renderWidth, renderHeight);

      FramebufferCreateInfo fbInfo = {};
      fbInfo.colorTexture = texBlackhole;
      fbInfo.width = renderWidth;
      fbInfo.height = renderHeight;
      fbInfo.createDepthBuffer = true;
      fboBlackhole = createFramebuffer(fbInfo);
      assert(fboBlackhole != 0);

      texBrightness = createColorTexture(renderWidth, renderHeight);
      texBloomFinal = createColorTexture(renderWidth, renderHeight);
      texTonemapped = createColorTexture(renderWidth, renderHeight);
      for (int i = 0; i < kMaxBloomIter; i++) {
        int downW = renderWidth >> (i + 1);
        int downH = renderHeight >> (i + 1);
        if (downW < 1)
          downW = 1;
        if (downH < 1)
          downH = 1;
        int upW = renderWidth >> i;
        int upH = renderHeight >> i;
        if (upW < 1)
          upW = 1;
        if (upH < 1)
          upH = 1;
        texDownsampled[i] = createColorTexture(downW, downH);
        texUpsampled[i] = createColorTexture(upW, upH);
      }
    }

    static bool mouseControlEnabled = true;
    static bool frontView = false;
    static bool topView = false;
    static float cameraRollDeg = 0.0f;
    const float fovScale = 1.0f;

    if (kEnableImGui) {
      ImGui::Checkbox("mouseControl", &mouseControlEnabled);
      ImGui::Checkbox("frontView", &frontView);
      ImGui::Checkbox("topView", &topView);
      ImGui::SliderFloat("cameraRoll", &cameraRollDeg, -180.0f, 180.0f);
    }

    // 使用缓动函数让动画更平滑
    float easedT = easeInOutCubic((float)autopilotT);
    glm::vec3 autopilotPos = calculateBezierPoint(easedT, bezierP0,
                                                  bezierP1, bezierP2, bezierP3);
    CameraState cameraState = computeCameraState(
        now, renderWidth, renderHeight, mouseX, mouseY, mouseControlEnabled,
        frontView, topView, cameraRollDeg, fovScale, autopilotActive,
        autopilotPos);
    {
      // --- Step 1: Black hole ray marching into fboBlackhole
      RenderToTextureInfo blackholeUniforms;
      RenderToTextureInfo &rtti = blackholeUniforms;
      rtti.cubemapUniforms["galaxy"] = galaxy;
      rtti.textureUniforms["colorMap"] = colorMap;
      rtti.floatUniforms["mouseX"] = mouseX;
      rtti.floatUniforms["mouseY"] = mouseY;

      IMGUI_TOGGLE(gravatationalLensing, true);
      IMGUI_TOGGLE(renderBlackHole, true);
      IMGUI_TOGGLE(adiskEnabled, true);
      IMGUI_TOGGLE(adiskParticle, true);
      IMGUI_SLIDER(adiskDensityV, 2.0f, 0.0f, 10.0f);
      IMGUI_SLIDER(adiskDensityH, 4.0f, 0.0f, 10.0f);
      IMGUI_SLIDER(adiskHeight, 0.55f, 0.0f, 1.0f);
      IMGUI_SLIDER(adiskLit, 0.25f, 0.0f, 4.0f);
      IMGUI_SLIDER(adiskNoiseLOD, 5.0f, 1.0f, 12.0f);
      IMGUI_SLIDER(adiskNoiseScale, 0.8f, 0.0f, 10.0f);
      IMGUI_SLIDER(adiskSpeed, 0.5f, 0.0f, 1.0f);

      rtti.floatUniforms["mouseControl"] = mouseControlEnabled ? 1.0f : 0.0f;
      rtti.floatUniforms["frontView"] = frontView ? 1.0f : 0.0f;
      rtti.floatUniforms["topView"] = topView ? 1.0f : 0.0f;
      rtti.floatUniforms["cameraRoll"] = cameraRollDeg;
      rtti.floatUniforms["fovScale"] = fovScale;
      rtti.floatUniforms["useExternalCamera"] = 1.0f;
      rtti.floatUniforms["externalFovScale"] = cameraState.fovScale;
      rtti.vec3Uniforms["externalCameraPos"] = cameraState.pos;
      rtti.vec3Uniforms["externalTarget"] = cameraState.target;

      glBindFramebuffer(GL_FRAMEBUFFER, fboBlackhole);
      glViewport(0, 0, renderWidth, renderHeight);
      glDisable(GL_DEPTH_TEST);

      glUseProgram(blackholeProgram);
      glBindVertexArray(quadVAO);

      glUniform2f(glGetUniformLocation(blackholeProgram, "resolution"),
                  (float)renderWidth, (float)renderHeight);
      glUniform1f(glGetUniformLocation(blackholeProgram, "time"), (float)now);

      for (auto const &[name, val] : rtti.floatUniforms) {
        GLint loc = glGetUniformLocation(blackholeProgram, name.c_str());
        if (loc != -1) {
          glUniform1f(loc, val);
        }
      }
      for (auto const &[name, val] : rtti.vec3Uniforms) {
        GLint loc = glGetUniformLocation(blackholeProgram, name.c_str());
        if (loc != -1) {
          glUniform3fv(loc, 1, glm::value_ptr(val));
        }
      }

      int textureUnit = 0;
      auto bindTexture = [&](const std::string &name, GLuint tex,
                             GLenum type) {
        GLint loc = glGetUniformLocation(blackholeProgram, name.c_str());
        if (loc != -1) {
          glUniform1i(loc, textureUnit);
          glActiveTexture(GL_TEXTURE0 + textureUnit);
          glBindTexture(type, tex);
          textureUnit++;
        }
      };
      for (auto const &[name, tex] : rtti.textureUniforms) {
        bindTexture(name, tex, GL_TEXTURE_2D);
      }
      for (auto const &[name, tex] : rtti.cubemapUniforms) {
        bindTexture(name, tex, GL_TEXTURE_CUBE_MAP);
      }

      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // --- Step 2: Depth pass for the satellite in the same FBO
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glm::vec3 satellitePos = glm::vec3(2.0f, -1.0f, -3.0f);
    glm::mat4 satelliteModel = computeSatelliteModel(now, satellitePos);
    glm::vec3 lightDir = glm::normalize(-satellitePos);
    renderSatellite(satelliteMesh, satelliteProgram, satelliteModel,
                    cameraState.view, cameraState.projection, cameraState.pos,
                    lightDir);

    // --- Step 4: release FBO and move into the bloom chain
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);

    {
      RenderToTextureInfo rtti;
      rtti.fragShader = "shader/bloom_brightness_pass.frag";
      rtti.textureUniforms["texture0"] = texBlackhole;
      rtti.targetTexture = texBrightness;
      rtti.width = renderWidth;
      rtti.height = renderHeight;
      renderToTexture(rtti);
    }

    static int bloomIterations = 5;  // Reduced from kMaxBloomIter for performance
    if (kEnableImGui) {
      ImGui::SliderInt("bloomIterations", &bloomIterations, 1, kMaxBloomIter);
    }
    for (int level = 0; level < bloomIterations; level++) {
      RenderToTextureInfo rtti;
      rtti.fragShader = "shader/bloom_downsample.frag";
      rtti.textureUniforms["texture0"] =
          level == 0 ? texBrightness : texDownsampled[level - 1];
      rtti.targetTexture = texDownsampled[level];
      rtti.width = renderWidth >> (level + 1);
      rtti.height = renderHeight >> (level + 1);
      if (rtti.width < 1)
        rtti.width = 1;
      if (rtti.height < 1)
        rtti.height = 1;
      renderToTexture(rtti);
    }

    for (int level = bloomIterations - 1; level >= 0; level--) {
      RenderToTextureInfo rtti;
      rtti.fragShader = "shader/bloom_upsample.frag";
      rtti.textureUniforms["texture0"] = level == bloomIterations - 1
                                             ? texDownsampled[level]
                                             : texUpsampled[level + 1];
      rtti.textureUniforms["texture1"] =
          level == 0 ? texBrightness : texDownsampled[level - 1];
      rtti.targetTexture = texUpsampled[level];
      rtti.width = renderWidth >> level;
      rtti.height = renderHeight >> level;
      if (rtti.width < 1)
        rtti.width = 1;
      if (rtti.height < 1)
        rtti.height = 1;
      renderToTexture(rtti);
    }

    {
      RenderToTextureInfo rtti;
      rtti.fragShader = "shader/bloom_composite.frag";
      rtti.textureUniforms["texture0"] = texBlackhole;
      rtti.textureUniforms["texture1"] = texUpsampled[0];
      rtti.targetTexture = texBloomFinal;
      rtti.width = renderWidth;
      rtti.height = renderHeight;

      IMGUI_SLIDER(bloomStrength, 0.1f, 0.0f, 1.0f);

      renderToTexture(rtti);
    }

    {
      RenderToTextureInfo rtti;
      rtti.fragShader = "shader/tonemapping.frag";
      rtti.textureUniforms["texture0"] = texBloomFinal;
      rtti.targetTexture = texTonemapped;
      rtti.width = renderWidth;
      rtti.height = renderHeight;

      IMGUI_TOGGLE(tonemappingEnabled, true);
      IMGUI_SLIDER(gamma, 2.5f, 1.0f, 4.0f);

      renderToTexture(rtti);
    }

    passthrough.render(texTonemapped, width, height);

    if (kEnableImGui) {
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    glfwSwapBuffers(window);
  }

  if (kEnableImGui) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
