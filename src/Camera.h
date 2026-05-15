#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ──────────────────────────────────────────────────────────────────────────────
//  2-D orthographic camera for the simulation view.
//
//  World coordinates are centred at (0,0).  The camera pans and zooms to let
//  the user navigate large simulations.
// ──────────────────────────────────────────────────────────────────────────────
class Camera {
public:
    Camera() = default;

    void setViewport(int width, int height);

    // Pan the camera by a delta in world units
    void pan(glm::vec2 delta);

    // Zoom around a pivot point in screen coordinates
    void zoomAt(float factor, glm::vec2 screenPivot);

    // Convert screen pixel position → world position
    glm::vec2 screenToWorld(glm::vec2 screen) const;

    // Convert world position → screen pixel position
    glm::vec2 worldToScreen(glm::vec2 world) const;

    // Matrices for shader upload
    glm::mat4 viewMatrix()       const;
    glm::mat4 projectionMatrix() const;
    glm::mat4 viewProjection()   const { return projectionMatrix() * viewMatrix(); }

    // Accessors
    glm::vec2 position() const { return m_position; }
    float     zoom()     const { return m_zoom; }
    int       width()    const { return m_width; }
    int       height()   const { return m_height; }

    // Aspect ratio
    float aspect() const {
        return m_height > 0 ? static_cast<float>(m_width) / m_height : 1.f;
    }

    // Half-extents of the visible world rectangle
    float halfW() const { return m_halfExtent * aspect(); }
    float halfH() const { return m_halfExtent; }

private:
    glm::vec2 m_position   = {0.f, 0.f};
    float     m_zoom       = 1.f;          // > 1 → zoomed in
    float     m_halfExtent = 20.f;         // base half-height in world units

    int m_width  = 1280;
    int m_height = 720;
};
