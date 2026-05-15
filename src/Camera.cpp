#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

void Camera::setViewport(int w, int h) {
    m_vpWidth  = w;
    m_vpHeight = h;
}

// ──────────────────────────────────────────────────────────────────────────────
//  Eye position from spherical coordinates around target.
//
//  Spherical layout (right-handed, Y-up):
//    yaw   = rotation around world Y
//    pitch = elevation angle; negative values look downward
//
//  At yaw=0, pitch=-pi/2  : eye is directly above target
//  At yaw=0, pitch=-0.7   : eye is above-and-behind at ~40 degrees
// ──────────────────────────────────────────────────────────────────────────────
glm::vec3 Camera::eyePosition() const {
    float cp = std::cos(m_pitch);   // > 0 for all valid pitch values
    float sp = std::sin(-m_pitch);  // positive because pitch is negative
    return target + glm::vec3(
        m_distance * cp * std::sin(m_yaw),
        m_distance * sp,
        m_distance * cp * std::cos(m_yaw)
    );
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(eyePosition(), target, glm::vec3(0.f, 1.f, 0.f));
}

glm::mat4 Camera::projectionMatrix() const {
    float aspect = (m_vpHeight > 0)
        ? static_cast<float>(m_vpWidth) / m_vpHeight : 1.f;
    return glm::perspective(glm::radians(m_fov), aspect, 0.5f, 800.f);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Orbit: rotate yaw and pitch based on mouse drag pixels
// ──────────────────────────────────────────────────────────────────────────────
void Camera::orbit(glm::vec2 delta) {
    m_yaw   += delta.x * 0.006f;
    m_pitch -= delta.y * 0.006f;
    m_pitch  = std::clamp(m_pitch, MIN_PITCH, MAX_PITCH);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Pan: slide target in the XZ ground plane.
//
//  "right" and "forward_ground" are derived from the current yaw so panning
//  always feels aligned with the screen axes regardless of view angle.
// ──────────────────────────────────────────────────────────────────────────────
void Camera::pan(glm::vec2 delta) {
    // Camera-right vector projected to XZ (horizontal plane)
    glm::vec3 right   = glm::normalize(glm::vec3( std::cos(m_yaw), 0.f, -std::sin(m_yaw)));
    // Camera-forward vector projected to XZ
    glm::vec3 forward = glm::normalize(glm::vec3(-std::sin(m_yaw), 0.f, -std::cos(m_yaw)));

    float sensitivity = m_distance * 0.0030f;

    // Negate x so dragging right pulls scene right (camera-left direction)
    target -= right   * delta.x * sensitivity;
    target -= forward * delta.y * sensitivity;
}

// ──────────────────────────────────────────────────────────────────────────────
//  Zoom: exponential distance scaling keeps it consistent at all distances
// ──────────────────────────────────────────────────────────────────────────────
void Camera::zoomBy(float factor) {
    m_distance = std::clamp(m_distance / factor, MIN_DISTANCE, MAX_DISTANCE);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Unproject a screen pixel to the Y=0 world plane.
//  Returns the XZ hit point as a 2-D simulation-space coordinate.
// ──────────────────────────────────────────────────────────────────────────────
glm::vec2 Camera::screenToGroundXZ(glm::vec2 screen) const {
    // NDC: map [0,w]x[0,h] to [-1,1]x[-1,1] (Y flipped)
    float ndcX = (2.f * screen.x / m_vpWidth)  - 1.f;
    float ndcY = 1.f - (2.f * screen.y / m_vpHeight);

    glm::mat4 invVP = glm::inverse(viewProjection());

    // Unproject near and far points
    glm::vec4 nearH = invVP * glm::vec4(ndcX, ndcY, -1.f, 1.f);
    glm::vec4 farH  = invVP * glm::vec4(ndcX, ndcY,  1.f, 1.f);

    glm::vec3 rayOrigin = glm::vec3(nearH) / nearH.w;
    glm::vec3 rayDir    = glm::normalize(glm::vec3(farH) / farH.w - rayOrigin);

    // Intersect ray with Y=0 plane:  rayOrigin.y + t * rayDir.y = 0
    if (std::abs(rayDir.y) < 1e-6f)
        return glm::vec2(rayOrigin.x, rayOrigin.z);

    float t = -rayOrigin.y / rayDir.y;
    if (t < 0.f) t = -t; // behind camera; just use absolute value as fallback

    glm::vec3 hit = rayOrigin + t * rayDir;
    return glm::vec2(hit.x, hit.z); // XZ -> sim XY
}
