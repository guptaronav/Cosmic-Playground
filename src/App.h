#pragma once

#include "Simulation.h"
#include "Renderer.h"
#include "Camera.h"
#include "Input.h"
#include "UI.h"
#include "Presets.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

// ──────────────────────────────────────────────────────────────────────────────
//  Top-level application: owns the window, GL context, and all subsystems.
//  Run the application with app.run() after app.init().
// ──────────────────────────────────────────────────────────────────────────────
class App {
public:
    App(int width, int height, const std::string& title);
    ~App();

    bool init();
    void run();

private:
    // ── Window ─────────────────────────────────────────────────────────────────
    GLFWwindow* m_window = nullptr;
    int         m_width  = 1280;
    int         m_height = 720;
    std::string m_title;

    // ── Subsystems ─────────────────────────────────────────────────────────────
    Simulation m_sim;
    Renderer   m_renderer;
    Camera     m_camera;
    Input      m_input;
    UI         m_ui;
    Presets    m_presets;

    // ── Timing ─────────────────────────────────────────────────────────────────
    double m_lastTime   = 0.0;
    double m_simTime    = 0.0;
    float  m_fps        = 0.f;
    int    m_frameCount = 0;
    double m_fpsTimer   = 0.0;

    // ── Internal ───────────────────────────────────────────────────────────────
    void processInput(bool imguiCapturing);
    void onFramebufferResize(int w, int h);
    void handleBodyPlacement(glm::vec2 mouseScreen);

    // GLFW static callbacks that forward to the app instance
    static void cb_framebuffer(GLFWwindow* w, int width, int height);
};
