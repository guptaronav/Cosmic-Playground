#pragma once

#include "Body.h"
#include "Camera.h"
#include "Simulation.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool init();
    void resize(int width, int height);
    void draw(const Simulation& sim, const Camera& cam, float timeSeconds);

    bool showTrails = true;
    bool showGrid   = true;
    bool showGlow   = true;

private:
    GLuint compileShader(GLenum type, const char* src);
    GLuint linkProgram(GLuint vert, GLuint frag);
    GLuint loadProgram(const std::string& vertPath, const std::string& fragPath);

    GLuint m_bodyProg       = 0;
    GLuint m_trailProg      = 0;
    GLuint m_gridProg       = 0;
    GLuint m_blackholeProg  = 0;
    GLuint m_accretionProg  = 0;

    GLuint m_quadVAO = 0, m_quadVBO = 0;
    GLuint m_trailVAO = 0, m_trailVBO = 0;
    GLuint m_gridVAO = 0, m_gridVBO = 0, m_gridEBO = 0;
    GLuint m_diskVAO = 0, m_diskVBO = 0;

    static constexpr size_t TRAIL_VBO_CAPACITY = 2048;
    static constexpr int    GRID_LINES  = 60;    // lines per axis (denser = more dramatic)
    static constexpr float  GRID_EXTENT = 60.f;  // world units each side

    int m_gridIndexCount = 0;
    int m_fbWidth  = 1280;
    int m_fbHeight = 720;

    bool initQuad();
    bool initTrail();
    bool initGrid();
    bool initDisk();

    void drawGrid(const Simulation& sim, const glm::mat4& VP, const glm::vec3& eye);
    void drawBody(const Body& b, const glm::mat4& V, const glm::mat4& P);
    void drawBlackHole(const Body& b, const glm::mat4& V, const glm::mat4& P, float t);
    void drawTrail(const Body& b, const glm::mat4& VP);
    void drawAccretionDisk(const Body& b, const glm::mat4& VP, float t);
};
