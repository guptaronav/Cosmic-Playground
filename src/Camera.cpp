#include "Camera.h"

void Camera::setViewport(int width, int height) {
    m_width  = width;
    m_height = height;
}

void Camera::pan(glm::vec2 delta) {
    m_position += delta;
}

void Camera::zoomAt(float factor, glm::vec2 screenPivot) {
    // World position under the pivot before zoom
    glm::vec2 worldBefore = screenToWorld(screenPivot);

    m_zoom = glm::clamp(m_zoom * factor, 0.05f, 50.f);

    // World position under the pivot after zoom (differs because halfExtent changed)
    glm::vec2 worldAfter = screenToWorld(screenPivot);

    // Compensate so the pivot stays fixed
    m_position += worldBefore - worldAfter;
}

glm::vec2 Camera::screenToWorld(glm::vec2 screen) const {
    // NDC: map [0,w] x [0,h] → [-1,1] x [1,-1]
    glm::vec2 ndc = {
         (screen.x / m_width)  * 2.f - 1.f,
        -(screen.y / m_height) * 2.f + 1.f
    };
    // Apply inverse view-projection
    float hw = halfW() / m_zoom;
    float hh = halfH() / m_zoom;
    return m_position + ndc * glm::vec2(hw, hh);
}

glm::vec2 Camera::worldToScreen(glm::vec2 world) const {
    glm::vec2 rel = world - m_position;
    float hw = halfW() / m_zoom;
    float hh = halfH() / m_zoom;
    glm::vec2 ndc = rel / glm::vec2(hw, hh);
    return {
        (ndc.x + 1.f) * 0.5f * m_width,
        (1.f - ndc.y) * 0.5f * m_height
    };
}

glm::mat4 Camera::viewMatrix() const {
    return glm::translate(glm::mat4(1.f), glm::vec3(-m_position, 0.f));
}

glm::mat4 Camera::projectionMatrix() const {
    float hw = halfW() / m_zoom;
    float hh = halfH() / m_zoom;
    return glm::ortho(-hw, hw, -hh, hh, -1.f, 1.f);
}
