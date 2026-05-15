#pragma once

#include "Body.h"
#include "Camera.h"
#include "Simulation.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

// ──────────────────────────────────────────────────────────────────────────────
//  Manages all OpenGL resources and drawing.
//
//  Each visual layer has its own shader program and VAO/VBO.
//  The renderer is intentionally decoupled from the simulation: it receives
//  const references so it cannot mutate physics state.
// ──────────────────────────────────────────────────────────────────────────────
class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    // Initialise all GPU resources; call once after a valid GL context exists
    bool init();

    // Resize the framebuffer (called from the GLFW framebuffer-resize callback)
    void resize(int width, int height);

    // Draw the full scene
    void draw(const Simulation& sim, const Camera& cam, float timeSeconds);

    // Visual toggles mirrored from UI
    bool showTrails  = true;
    bool showGrid    = true;
    bool showGlow    = true;

private:
    // ── Shader helpers ────────────────────────────────────────────────────────
    GLuint compileShader(GLenum type, const char* src);
    GLuint linkProgram(GLuint vert, GLuint frag);
    GLuint loadProgram(const std::string& vertPath, const std::string& fragPath);

    // ── Programs ──────────────────────────────────────────────────────────────
    GLuint m_bodyProg       = 0;
    GLuint m_trailProg      = 0;
    GLuint m_gridProg       = 0;
    GLuint m_blackholeProg  = 0;
    GLuint m_accretionProg  = 0;

    // ── Body / planet quad ────────────────────────────────────────────────────
    GLuint m_quadVAO = 0, m_quadVBO = 0;

    // ── Trail line strip ──────────────────────────────────────────────────────
    GLuint m_trailVAO = 0, m_trailVBO = 0;
    static constexpr size_t TRAIL_VBO_CAPACITY = 2048;

    // ── Spacetime grid ────────────────────────────────────────────────────────
    GLuint m_gridVAO = 0, m_gridVBO = 0, m_gridEBO = 0;
    int    m_gridIndexCount = 0;
    static constexpr int GRID_LINES = 40;  // lines per axis

    // ── Accretion disk ────────────────────────────────────────────────────────
    GLuint m_diskVAO = 0, m_diskVBO = 0;

    int m_fbWidth  = 1280;
    int m_fbHeight = 720;

    // ── Initialisation sub-routines ───────────────────────────────────────────
    bool initQuad();
    bool initTrail();
    bool initGrid();
    bool initDisk();

    // ── Draw sub-routines ─────────────────────────────────────────────────────
    void drawGrid(const Simulation& sim, const Camera& cam);
    void drawBody(const Body& b, const Camera& cam);
    void drawBlackHole(const Body& b, const Camera& cam, float t);
    void drawTrail(const Body& b, const Camera& cam);
    void drawAccretionDisk(const Body& b, const Camera& cam, float t);
};
