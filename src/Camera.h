#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ──────────────────────────────────────────────────────────────────────────────
//  Orbit camera around a target point in 3D space.
//
//  Coordinate convention
//    Simulation 2-D positions (sx, sy)  ->  world 3-D positions (sx, 0, sy)
//    World Y is the vertical axis (up).
//    The camera orbits above the XZ ground plane.
//
//  Controls (wired in App)
//    Right-click drag  : orbit (yaw / pitch)
//    Middle-click drag : pan target in world XZ
//    Scroll            : zoom (adjust distance)
// ──────────────────────────────────────────────────────────────────────────────
class Camera {
public:
    Camera() = default;

    void setViewport(int width, int height);

    // Orbit: pixelDelta from mouse drag
    void orbit(glm::vec2 pixelDelta);

    // Pan: moves target in world XZ, screen-space delta
    void pan(glm::vec2 pixelDelta);

    // Zoom: factor > 1 zooms in
    void zoomBy(float factor);

    // Unproject screen pixel to the Y=0 ground plane (returns sim-space XY)
    glm::vec2 screenToGroundXZ(glm::vec2 screen) const;

    // Matrices
    glm::mat4 viewMatrix()       const;
    glm::mat4 projectionMatrix() const;
    glm::mat4 viewProjection()   const { return projectionMatrix() * viewMatrix(); }

    // Eye position in world space
    glm::vec3 eyePosition() const;

    // Viewport dimensions
    int width()  const { return m_vpWidth;  }
    int height() const { return m_vpHeight; }

    // Expose for UI
    float distance() const { return m_distance; }
    float yaw()      const { return m_yaw;      }
    float pitch()    const { return m_pitch;     }

    // Camera state (public so UI can tweak/display)
    glm::vec3 target   = {0.f, 0.f, 0.f}; // look-at point in world XZ
    float m_distance   = 50.f;             // distance from target
    float m_yaw        = 0.f;              // horizontal rotation (radians)
    float m_pitch      = -0.70f;           // vertical tilt; -pi/2 = straight down
    float m_fov        = 55.f;             // vertical FOV in degrees

private:
    int m_vpWidth  = 1280;
    int m_vpHeight = 720;

    static constexpr float MIN_PITCH    = -1.50f;
    static constexpr float MAX_PITCH    = -0.08f;
    static constexpr float MIN_DISTANCE =  4.f;
    static constexpr float MAX_DISTANCE = 250.f;
};
