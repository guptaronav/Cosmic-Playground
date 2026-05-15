#include "App.h"
#include "Presets.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
#include <cmath>

static App* g_app = nullptr;

void App::cb_framebuffer(GLFWwindow*, int w, int h) {
    if (g_app) g_app->onFramebufferResize(w, h);
}

App::App(int width, int height, const std::string& title)
    : m_width(width), m_height(height), m_title(title) {}

App::~App() {
    m_ui.shutdown();
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool App::init() {
    if (!glfwInit()) { std::cerr << "[App] GLFW init failed\n"; return false; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window) { std::cerr << "[App] Window creation failed\n"; glfwTerminate(); return false; }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "[App] GLAD load failed\n"; return false;
    }
    std::cout << "[App] OpenGL " << glGetString(GL_VERSION) << "\n";

    g_app = this;
    glfwSetFramebufferSizeCallback(m_window, cb_framebuffer);
    m_input.install(m_window);

    int fbw, fbh;
    glfwGetFramebufferSize(m_window, &fbw, &fbh);
    m_camera.setViewport(fbw, fbh);
    m_renderer.resize(fbw, fbh);

    if (!m_renderer.init()) { std::cerr << "[App] Renderer init failed\n"; return false; }

    m_ui.init(m_window);
    m_presets.apply(0, m_sim);  // Solar System

    m_lastTime = glfwGetTime();
    return true;
}

void App::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        double now = glfwGetTime();
        double dt  = std::min(now - m_lastTime, 0.05);
        m_lastTime = now;

        // FPS counter
        m_frameCount++;
        m_fpsTimer += dt;
        if (m_fpsTimer >= 0.5) {
            m_fps        = static_cast<float>(m_frameCount / m_fpsTimer);
            m_frameCount = 0;
            m_fpsTimer   = 0.0;
        }

        m_ui.beginFrame();
        bool imguiCapturing = m_ui.draw(m_sim, m_renderer, m_camera,
                                         m_presets, m_fps,
                                         static_cast<float>(m_simTime));
        processInput(imguiCapturing);

        m_renderer.showTrails = m_sim.config().trailsEnabled;
        m_renderer.showGrid   = m_sim.config().gridEnabled;

        m_sim.update(dt);
        if (!m_sim.config().paused) m_simTime += dt * m_sim.config().timeScale;

        m_renderer.draw(m_sim, m_camera, static_cast<float>(glfwGetTime()));
        m_ui.endFrame();

        glfwSwapBuffers(m_window);
        m_input.endFrame();
    }
}

void App::processInput(bool imguiCapturing) {
    // ── Global keyboard shortcuts ──────────────────────────────────────────────
    if (m_input.wasKeyPressed(GLFW_KEY_SPACE))
        m_sim.config().paused = !m_sim.config().paused;

    if (m_input.wasKeyPressed(GLFW_KEY_R))
        m_sim.clearBodies();

    if (m_input.wasKeyPressed(GLFW_KEY_T)) {
        m_sim.config().trailsEnabled = !m_sim.config().trailsEnabled;
        m_renderer.showTrails        = m_sim.config().trailsEnabled;
    }
    if (m_input.wasKeyPressed(GLFW_KEY_G)) {
        m_sim.config().gridEnabled = !m_sim.config().gridEnabled;
        m_renderer.showGrid        = m_sim.config().gridEnabled;
    }

    // Home key: reset camera to default view
    if (m_input.wasKeyPressed(GLFW_KEY_HOME) || m_input.wasKeyPressed(GLFW_KEY_C)) {
        m_camera.target     = glm::vec3(0.f);
        m_camera.m_distance = 50.f;
        m_camera.m_yaw      = 0.f;
        m_camera.m_pitch    = -0.70f;
    }

    if (m_input.wasKeyPressed(GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);

    // ── WASD / arrow key pan ───────────────────────────────────────────────────
    const float PAN_SPEED = 22.f;
    glm::vec2   kbPan     = {0.f, 0.f};
    if (m_input.isKeyDown(GLFW_KEY_A) || m_input.isKeyDown(GLFW_KEY_LEFT))  kbPan.x -= PAN_SPEED;
    if (m_input.isKeyDown(GLFW_KEY_D) || m_input.isKeyDown(GLFW_KEY_RIGHT)) kbPan.x += PAN_SPEED;
    if (m_input.isKeyDown(GLFW_KEY_W) || m_input.isKeyDown(GLFW_KEY_UP))    kbPan.y -= PAN_SPEED;
    if (m_input.isKeyDown(GLFW_KEY_S) || m_input.isKeyDown(GLFW_KEY_DOWN))  kbPan.y += PAN_SPEED;
    if (kbPan != glm::vec2{0.f, 0.f})
        m_camera.pan(kbPan * static_cast<float>(1.0 / 60.0));

    if (imguiCapturing) return;

    // ── Scroll to zoom ─────────────────────────────────────────────────────────
    float scroll = m_input.scrollDelta();
    if (std::abs(scroll) > 0.001f)
        m_camera.zoomBy(scroll > 0.f ? 1.12f : (1.f / 1.12f));

    // ── Right-click drag: orbit ────────────────────────────────────────────────
    if (m_input.isMouseDown(GLFW_MOUSE_BUTTON_RIGHT) && !m_input.isShiftHeld()) {
        m_camera.orbit(m_input.mouseDelta());
    }

    // ── Middle-click drag OR Alt + left: pan ──────────────────────────────────
    bool panDrag = m_input.isMouseDown(GLFW_MOUSE_BUTTON_MIDDLE) ||
                   (m_input.isMouseDown(GLFW_MOUSE_BUTTON_LEFT) && m_input.isCtrlHeld());
    if (panDrag)
        m_camera.pan(m_input.mouseDelta());

    // ── Left-click: select body ────────────────────────────────────────────────
    if (m_input.wasMousePressed(GLFW_MOUSE_BUTTON_LEFT) && !m_input.isShiftHeld() && !m_input.isCtrlHeld()) {
        glm::vec2 worldXZ = m_camera.screenToGroundXZ(m_input.mousePos());
        Body* picked = m_sim.pickBody(glm::dvec2(worldXZ), 2.5);
        m_ui.selectedBodyId = picked ? picked->id : 0;
    }

    // ── Shift + left-click: place body ────────────────────────────────────────
    if (m_input.wasMousePressed(GLFW_MOUSE_BUTTON_LEFT) && m_input.isShiftHeld()) {
        handleBodyPlacement(m_input.mousePos());
    }

    // ── Shift + right-click drag: set launch velocity ─────────────────────────
    if (m_input.wasMousePressed(GLFW_MOUSE_BUTTON_RIGHT) && m_input.isShiftHeld()) {
        m_ui.velocityDrag = true;
        m_ui.launchStart  = m_camera.screenToGroundXZ(m_input.mousePos());
    }
    if (!m_input.isMouseDown(GLFW_MOUSE_BUTTON_RIGHT))
        m_ui.velocityDrag = false;
}

void App::handleBodyPlacement(glm::vec2 mouseScreen) {
    glm::vec2 worldXZ = m_camera.screenToGroundXZ(mouseScreen);
    Body b            = m_ui.pendingBody;
    b.position        = glm::dvec2(worldXZ);

    if (m_ui.velocityDrag) {
        glm::vec2 cur = m_camera.screenToGroundXZ(m_input.mousePos());
        b.velocity    = glm::dvec2((m_ui.launchStart - cur) * 5.f);
    }
    m_sim.addBody(b);
}

void App::onFramebufferResize(int w, int h) {
    m_camera.setViewport(w, h);
    m_renderer.resize(w, h);
}
