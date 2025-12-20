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

#include <assert.h>
#include <map>
#include <stdio.h>
#include <vector>

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
static const int kMaxBloomIter = 8;

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

void mouseCallback(GLFWwindow *window, double x, double y) {
  static float lastX = 400.0f;
  static float lastY = 300.0f;
  static float yaw = 0.0f;
  static float pitch = 0.0f;
  static float firstMouse = true;

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

    glUniform2f(glGetUniformLocation(this->program, "resolution"),
                (float)width, (float)height);

    glUniform1f(glGetUniformLocation(this->program, "time"),
                (float)glfwGetTime());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputColorTexture);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glUseProgram(0);
  }
};

int main(int, char **) {
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
  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

  GLuint fboBlackhole, texBlackhole;
  texBlackhole = createColorTexture(SCR_WIDTH, SCR_HEIGHT);

  FramebufferCreateInfo info = {};
  info.colorTexture = texBlackhole;
  if (!(fboBlackhole = createFramebuffer(info))) {
    assert(false);
  }

  GLuint quadVAO = createQuadVAO();
  glBindVertexArray(quadVAO);

  // Main loop
  PostProcessPass passthrough("shader/passthrough.frag");

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

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
    static GLuint uvChecker = loadTexture2D("assets/uv_checker.png");

    static int renderWidth = 0;
    static int renderHeight = 0;
    static GLuint texBlackhole = 0;
    static GLuint texBrightness = 0;
    static GLuint texDownsampled[kMaxBloomIter] = {};
    static GLuint texUpsampled[kMaxBloomIter] = {};
    static GLuint texBloomFinal = 0;
    static GLuint texTonemapped = 0;
    if (width != renderWidth || height != renderHeight) {
      renderWidth = width;
      renderHeight = height;
      SCR_WIDTH = width;
      SCR_HEIGHT = height;

      texBlackhole = createColorTexture(renderWidth, renderHeight);
      texBrightness = createColorTexture(renderWidth, renderHeight);
      texBloomFinal = createColorTexture(renderWidth, renderHeight);
      texTonemapped = createColorTexture(renderWidth, renderHeight);
      for (int i = 0; i < kMaxBloomIter; i++) {
        int downW = renderWidth >> (i + 1);
        int downH = renderHeight >> (i + 1);
        if (downW < 1) downW = 1;
        if (downH < 1) downH = 1;
        int upW = renderWidth >> i;
        int upH = renderHeight >> i;
        if (upW < 1) upW = 1;
        if (upH < 1) upH = 1;
        texDownsampled[i] = createColorTexture(downW, downH);
        texUpsampled[i] = createColorTexture(upW, upH);
      }
    }
    {
      RenderToTextureInfo rtti;
      rtti.fragShader = "shader/blackhole_main.frag";
      rtti.cubemapUniforms["galaxy"] = galaxy;
      rtti.textureUniforms["colorMap"] = colorMap;
      rtti.floatUniforms["mouseX"] = mouseX;
      rtti.floatUniforms["mouseY"] = mouseY;
      rtti.targetTexture = texBlackhole;
      rtti.width = renderWidth;
      rtti.height = renderHeight;

      IMGUI_TOGGLE(gravatationalLensing, true);
      IMGUI_TOGGLE(renderBlackHole, true);
      IMGUI_TOGGLE(mouseControl, true);
      IMGUI_SLIDER(cameraRoll, 0.0f, -180.0f, 180.0f);
      IMGUI_TOGGLE(frontView, false);
      IMGUI_TOGGLE(topView, false);
      IMGUI_TOGGLE(adiskEnabled, true);
      IMGUI_TOGGLE(adiskParticle, true);
      IMGUI_SLIDER(adiskDensityV, 2.0f, 0.0f, 10.0f);
      IMGUI_SLIDER(adiskDensityH, 4.0f, 0.0f, 10.0f);
      IMGUI_SLIDER(adiskHeight, 0.55f, 0.0f, 1.0f);
      IMGUI_SLIDER(adiskLit, 0.25f, 0.0f, 4.0f);
      IMGUI_SLIDER(adiskNoiseLOD, 5.0f, 1.0f, 12.0f);
      IMGUI_SLIDER(adiskNoiseScale, 0.8f, 0.0f, 10.0f);
      IMGUI_SLIDER(adiskSpeed, 0.5f, 0.0f, 1.0f);

      renderToTexture(rtti);
    }

    {
      RenderToTextureInfo rtti;
      rtti.fragShader = "shader/bloom_brightness_pass.frag";
      rtti.textureUniforms["texture0"] = texBlackhole;
      rtti.targetTexture = texBrightness;
      rtti.width = renderWidth;
      rtti.height = renderHeight;
      renderToTexture(rtti);
    }

    static int bloomIterations = kMaxBloomIter;
    if (kEnableImGui) {
      ImGui::SliderInt("bloomIterations", &bloomIterations, 1, 8);
    }
    for (int level = 0; level < bloomIterations; level++) {
      RenderToTextureInfo rtti;
      rtti.fragShader = "shader/bloom_downsample.frag";
      rtti.textureUniforms["texture0"] =
          level == 0 ? texBrightness : texDownsampled[level - 1];
      rtti.targetTexture = texDownsampled[level];
      rtti.width = renderWidth >> (level + 1);
      rtti.height = renderHeight >> (level + 1);
      if (rtti.width < 1) rtti.width = 1;
      if (rtti.height < 1) rtti.height = 1;
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
      if (rtti.width < 1) rtti.width = 1;
      if (rtti.height < 1) rtti.height = 1;
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
