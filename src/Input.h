#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <functional>
#include <unordered_map>

struct GLFWwindow;

// ──────────────────────────────────────────────────────────────────────────────
//  Thin wrapper around GLFW input state.
//
//  The App installs GLFW callbacks that forward events here.  Other systems
//  poll isKeyDown() / wasKeyPressed() / etc. each frame.
// ──────────────────────────────────────────────────────────────────────────────
class Input {
public:
    // Install GLFW callbacks on the given window (call once after window creation)
    void install(GLFWwindow* window);

    // Call at the END of each frame to advance pressed/released state
    void endFrame();

    // ── Keyboard ──────────────────────────────────────────────────────────────
    bool isKeyDown(int glfwKey)     const;
    bool wasKeyPressed(int glfwKey) const;   // true only on the frame it was first pressed
    bool wasKeyReleased(int glfwKey) const;

    // ── Mouse ─────────────────────────────────────────────────────────────────
    bool  isMouseDown(int button)     const;
    bool  wasMousePressed(int button)  const;
    bool  wasMouseReleased(int button) const;

    glm::vec2 mousePos()   const { return m_mousePos;   }
    glm::vec2 mouseDelta() const { return m_mouseDelta; }
    float     scrollDelta() const { return m_scrollDelta; }

    bool isShiftHeld() const { return isKeyDown(GLFW_KEY_LEFT_SHIFT) || isKeyDown(GLFW_KEY_RIGHT_SHIFT); }
    bool isCtrlHeld()  const { return isKeyDown(GLFW_KEY_LEFT_CONTROL) || isKeyDown(GLFW_KEY_RIGHT_CONTROL); }

    // Raw GLFW callbacks (public so they can be called from static C callbacks)
    void onKey(int key, int action);
    void onMouseButton(int button, int action);
    void onMouseMove(double x, double y);
    void onScroll(double xoff, double yoff);

private:
    // Key state: 0=up, 1=just pressed, 2=held, 3=just released
    std::unordered_map<int, int> m_keyState;
    std::unordered_map<int, int> m_mouseState;

    glm::vec2 m_mousePos      = {0.f, 0.f};
    glm::vec2 m_mousePrevPos  = {0.f, 0.f};
    glm::vec2 m_mouseDelta    = {0.f, 0.f};
    float     m_scrollDelta   = 0.f;

    bool firstMouse = true;
};
