#include "Input.h"
#include <GLFW/glfw3.h>

// ── Static bridge callbacks ────────────────────────────────────────────────────
static Input* g_input = nullptr;

static void cb_key(GLFWwindow*, int key, int /*scan*/, int action, int /*mods*/) {
    if (g_input) g_input->onKey(key, action);
}
static void cb_button(GLFWwindow*, int button, int action, int /*mods*/) {
    if (g_input) g_input->onMouseButton(button, action);
}
static void cb_move(GLFWwindow*, double x, double y) {
    if (g_input) g_input->onMouseMove(x, y);
}
static void cb_scroll(GLFWwindow*, double xoff, double yoff) {
    if (g_input) g_input->onScroll(xoff, yoff);
}

void Input::install(GLFWwindow* window) {
    g_input = this;
    glfwSetKeyCallback(window, cb_key);
    glfwSetMouseButtonCallback(window, cb_button);
    glfwSetCursorPosCallback(window, cb_move);
    glfwSetScrollCallback(window, cb_scroll);
}

// ── Event handlers ────────────────────────────────────────────────────────────

void Input::onKey(int key, int action) {
    if      (action == GLFW_PRESS)   m_keyState[key] = 1;
    else if (action == GLFW_RELEASE) m_keyState[key] = 3;
    // GLFW_REPEAT: we leave the state as "held" (2)
}

void Input::onMouseButton(int button, int action) {
    if      (action == GLFW_PRESS)   m_mouseState[button] = 1;
    else if (action == GLFW_RELEASE) m_mouseState[button] = 3;
}

void Input::onMouseMove(double x, double y) {
    if (firstMouse) {
        m_mousePrevPos = {static_cast<float>(x), static_cast<float>(y)};
        firstMouse = false;
    }
    m_mousePos = {static_cast<float>(x), static_cast<float>(y)};
}

void Input::onScroll(double /*xoff*/, double yoff) {
    m_scrollDelta += static_cast<float>(yoff);
}

// ── Per-frame advance ──────────────────────────────────────────────────────────

void Input::endFrame() {
    m_mouseDelta   = m_mousePos - m_mousePrevPos;
    m_mousePrevPos = m_mousePos;
    m_scrollDelta  = 0.f;

    // Advance state machine: just-pressed→held, just-released→up
    for (auto& [k, s] : m_keyState) {
        if (s == 1) s = 2;
        else if (s == 3) s = 0;
    }
    for (auto& [b, s] : m_mouseState) {
        if (s == 1) s = 2;
        else if (s == 3) s = 0;
    }
}

// ── Queries ───────────────────────────────────────────────────────────────────

bool Input::isKeyDown(int key) const {
    auto it = m_keyState.find(key);
    return it != m_keyState.end() && (it->second == 1 || it->second == 2);
}

bool Input::wasKeyPressed(int key) const {
    auto it = m_keyState.find(key);
    return it != m_keyState.end() && it->second == 1;
}

bool Input::wasKeyReleased(int key) const {
    auto it = m_keyState.find(key);
    return it != m_keyState.end() && it->second == 3;
}

bool Input::isMouseDown(int button) const {
    auto it = m_mouseState.find(button);
    return it != m_mouseState.end() && (it->second == 1 || it->second == 2);
}

bool Input::wasMousePressed(int button) const {
    auto it = m_mouseState.find(button);
    return it != m_mouseState.end() && it->second == 1;
}

bool Input::wasMouseReleased(int button) const {
    auto it = m_mouseState.find(button);
    return it != m_mouseState.end() && it->second == 3;
}
