#include "App.h"
#include "Presets.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
#include <cmath>

// ── GLFW static bridge ──────────────────────────────────────────────────────────
static App* g_app = nullptr;

void App::cb_framebuffer(GLFWwindow*, int w, int h) {
    if (g_app) g_app->onFramebufferResize(w, h);
}

// ── Constructor / Destructor ────────────────────────────────────────────────────
App::App(int width, int height, const std::string& title)
    : m_width(width), m_height(height), m_title(title) {}

App::~App() {
    m_ui.shutdown();
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

// ── Initialisation ──────────────────────────────────────────────────────────────
bool App::init() {
    // ── GLFW ──────────────────────────────────────────────────────────────────
    if (!glfwInit()) {
        std::cerr << "[App] Failed to initialise GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "[App] Failed to create window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // vsync

    // ── GLAD ──────────────────────────────────────────────────────────────────
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "[App] Failed to load OpenGL via GLAD\n";
        return false;
    }
    std::cout << "[App] OpenGL " << glGetString(GL_VERSION) << "\n";

    // ── Callbacks ─────────────────────────────────────────────────────────────
    g_app = this;
    glfwSetFramebufferSizeCallback(m_window, cb_framebuffer);

    // ── Input ─────────────────────────────────────────────────────────────────
    m_input.install(m_window);

    // ── Camera ────────────────────────────────────────────────────────────────
    int fbw, fbh;
    glfwGetFramebufferSize(m_window, &fbw, &fbh);
    m_camera.setViewport(fbw, fbh);

    // ── Renderer ──────────────────────────────────────────────────────────────
    m_renderer.resize(fbw, fbh);
    if (!m_renderer.init()) {
        std::cerr << "[App] Renderer init failed\n";
        return false;
    }

    // ── UI ────────────────────────────────────────────────────────────────────
    m_ui.init(m_window);

    // ── Default preset ────────────────────────────────────────────────────────
    m_presets.apply(0, m_sim); // Solar System

    m_lastTime = glfwGetTime();
    return true;
}

// ── Main loop ───────────────────────────────────────────────────────────────────
void App::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        // ── Timing ────────────────────────────────────────────────────────────
        double now = glfwGetTime();
        double dt  = now - m_lastTime;
        m_lastTime = now;

        // Cap dt to avoid physics explosion when window is dragged / minimised
        dt = std::min(dt, 0.05);

        // FPS smoothing
        m_frameCount++;
        m_fpsTimer += dt;
        if (m_fpsTimer >= 0.5) {
            m_fps        = static_cast<float>(m_frameCount / m_fpsTimer);
            m_frameCount = 0;
            m_fpsTimer   = 0.0;
        }

        // ── ImGui frame start ─────────────────────────────────────────────────
        m_ui.beginFrame();

        // ── UI panels ─────────────────────────────────────────────────────────
        bool imguiCapturing = m_ui.draw(m_sim, m_renderer, m_camera,
                                         m_presets, m_fps,
                                         static_cast<float>(m_simTime));

        // ── Input processing ──────────────────────────────────────────────────
        processInput(imguiCapturing);

        // ── Sync visual toggles from sim config ───────────────────────────────
        m_renderer.showTrails = m_sim.config().trailsEnabled;
        m_renderer.showGrid   = m_sim.config().gridEnabled;

        // ── Physics ───────────────────────────────────────────────────────────
        m_sim.update(dt);
        if (!m_sim.config().paused)
            m_simTime += dt * m_sim.config().timeScale;

        // ── Render ────────────────────────────────────────────────────────────
        m_renderer.draw(m_sim, m_camera, static_cast<float>(glfwGetTime()));

        // ── ImGui render ──────────────────────────────────────────────────────
        m_ui.endFrame();

        glfwSwapBuffers(m_window);
        m_input.endFrame();
    }
}

// ── Input ───────────────────────────────────────────────────────────────────────
void App::processInput(bool imguiCapturing) {
    // ── Keyboard shortcuts (always active) ────────────────────────────────────
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

    if (m_input.wasKeyPressed(GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);

    // ── Camera pan with WASD / arrow keys ─────────────────────────────────────
    const float panSpeed = 15.f / m_camera.zoom();
    float       dt_pan   = 1.f / 60.f; // fixed approximate delta for panning
    glm::vec2   panDelta = {0.f, 0.f};
    if (m_input.isKeyDown(GLFW_KEY_A) || m_input.isKeyDown(GLFW_KEY_LEFT))  panDelta.x -= panSpeed * dt_pan;
    if (m_input.isKeyDown(GLFW_KEY_D) || m_input.isKeyDown(GLFW_KEY_RIGHT)) panDelta.x += panSpeed * dt_pan;
    if (m_input.isKeyDown(GLFW_KEY_S) || m_input.isKeyDown(GLFW_KEY_DOWN))  panDelta.y -= panSpeed * dt_pan;
    if (m_input.isKeyDown(GLFW_KEY_W) || m_input.isKeyDown(GLFW_KEY_UP))    panDelta.y += panSpeed * dt_pan;
    if (panDelta != glm::vec2{0.f, 0.f})
        m_camera.pan(panDelta);

    // Stop here if ImGui captured the mouse
    if (imguiCapturing) return;

    // ── Scroll to zoom ────────────────────────────────────────────────────────
    float scroll = m_input.scrollDelta();
    if (std::abs(scroll) > 0.001f) {
        float factor = (scroll > 0.f) ? 1.12f : (1.f / 1.12f);
        m_camera.zoomAt(factor, m_input.mousePos());
    }

    // ── Middle mouse drag to pan ───────────────────────────────────────────────
    if (m_input.isMouseDown(GLFW_MOUSE_BUTTON_MIDDLE)) {
        glm::vec2 delta = m_input.mouseDelta();
        // Convert pixel delta to world delta
        float worldPerPixel = (m_camera.halfW() * 2.f) / m_camera.width() / m_camera.zoom();
        m_camera.pan({-delta.x * worldPerPixel, delta.y * worldPerPixel});
    }

    // ── Left click: select or place body ──────────────────────────────────────
    if (m_input.wasMousePressed(GLFW_MOUSE_BUTTON_LEFT)) {
        glm::vec2 worldPos = m_camera.screenToWorld(m_input.mousePos());

        if (m_input.isShiftHeld()) {
            // Place a new body
            handleBodyPlacement(m_input.mousePos());
        } else {
            // Select nearest body
            Body* picked = m_sim.pickBody(glm::dvec2(worldPos), 2.0);
            m_ui.selectedBodyId = picked ? picked->id : 0;
        }
    }

    // ── Right click drag: set launch velocity (when in add mode) ──────────────
    if (m_input.wasMousePressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        m_ui.velocityDrag = true;
        m_ui.launchStart  = m_camera.screenToWorld(m_input.mousePos());
    }
    if (!m_input.isMouseDown(GLFW_MOUSE_BUTTON_RIGHT) && m_ui.velocityDrag) {
        m_ui.velocityDrag = false;
    }
}

void App::handleBodyPlacement(glm::vec2 mouseScreen) {
    glm::vec2  worldPos = m_camera.screenToWorld(mouseScreen);
    Body       b        = m_ui.pendingBody;
    b.position          = glm::dvec2(worldPos);

    // If right button is held, use drag vector as initial velocity
    if (m_input.isMouseDown(GLFW_MOUSE_BUTTON_RIGHT)) {
        glm::vec2 cur = m_camera.screenToWorld(m_input.mousePos());
        glm::vec2 vel = (m_ui.launchStart - cur) * 5.f;
        b.velocity    = glm::dvec2(vel);
    }

    m_sim.addBody(b);
}

void App::onFramebufferResize(int w, int h) {
    m_camera.setViewport(w, h);
    m_renderer.resize(w, h);
}
