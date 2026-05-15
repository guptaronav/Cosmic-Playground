#include "Renderer.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <vector>
#include <array>

// ── Shader source loader ───────────────────────────────────────────────────────
static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[Renderer] Cannot open shader: " << path << "\n";
        return "";
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint Renderer::compileShader(GLenum type, const char* src) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    GLint ok; glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[1024];
        glGetShaderInfoLog(id, sizeof(buf), nullptr, buf);
        std::cerr << "[Renderer] Shader compile error:\n" << buf << "\n";
        glDeleteShader(id);
        return 0;
    }
    return id;
}

GLuint Renderer::linkProgram(GLuint vert, GLuint frag) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[1024];
        glGetProgramInfoLog(prog, sizeof(buf), nullptr, buf);
        std::cerr << "[Renderer] Program link error:\n" << buf << "\n";
        glDeleteProgram(prog);
        return 0;
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

GLuint Renderer::loadProgram(const std::string& vertPath, const std::string& fragPath) {
    std::string vs = readFile(vertPath);
    std::string fs = readFile(fragPath);
    if (vs.empty() || fs.empty()) return 0;

    GLuint v = compileShader(GL_VERTEX_SHADER,   vs.c_str());
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs.c_str());
    if (!v || !f) return 0;
    return linkProgram(v, f);
}

// ── Init ───────────────────────────────────────────────────────────────────────
bool Renderer::init() {
    // Enable blending for glows and trails
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // Load all shader programs
    m_bodyProg      = loadProgram("shaders/body.vert",           "shaders/body.frag");
    m_trailProg     = loadProgram("shaders/trail.vert",          "shaders/trail.frag");
    m_gridProg      = loadProgram("shaders/grid.vert",           "shaders/grid.frag");
    m_blackholeProg = loadProgram("shaders/blackhole.vert",      "shaders/blackhole.frag");
    m_accretionProg = loadProgram("shaders/accretion_disk.vert", "shaders/accretion_disk.frag");

    if (!m_bodyProg || !m_trailProg || !m_gridProg ||
        !m_blackholeProg || !m_accretionProg) {
        return false;
    }

    return initQuad() && initTrail() && initGrid() && initDisk();
}

// ── Unit quad for body rendering ───────────────────────────────────────────────
bool Renderer::initQuad() {
    // 2×2 quad as a triangle strip: BL, BR, TL, TR
    float verts[] = {
        // pos        // uv
        -1.f, -1.f,   0.f, 0.f,
         1.f, -1.f,   1.f, 0.f,
        -1.f,  1.f,   0.f, 1.f,
         1.f,  1.f,   1.f, 1.f,
    };
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glBindVertexArray(0);
    return true;
}

// ── Trail line buffer ──────────────────────────────────────────────────────────
bool Renderer::initTrail() {
    glGenVertexArrays(1, &m_trailVAO);
    glGenBuffers(1, &m_trailVBO);
    glBindVertexArray(m_trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_trailVBO);
    // Reserve space: (x, y, alpha) per point
    glBufferData(GL_ARRAY_BUFFER, TRAIL_VBO_CAPACITY * 3 * sizeof(float),
                 nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)(2*sizeof(float)));
    glBindVertexArray(0);
    return true;
}

// ── Spacetime grid mesh ────────────────────────────────────────────────────────
bool Renderer::initGrid() {
    const int N = GRID_LINES + 1; // vertices per axis
    const float ext = 50.f;       // world-space half-extent of the grid

    std::vector<float> verts;
    verts.reserve(N * N * 2);

    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i) {
            float x = -ext + 2.f * ext * i / (N - 1);
            float y = -ext + 2.f * ext * j / (N - 1);
            verts.push_back(x);
            verts.push_back(y);
        }
    }

    // Index buffer: horizontal and vertical line strips
    std::vector<uint32_t> indices;
    // Horizontal lines
    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i)
            indices.push_back(j * N + i);
        if (j < N - 1) indices.push_back(0xFFFFFFFF); // primitive restart
    }
    // Vertical lines
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j)
            indices.push_back(j * N + i);
        if (i < N - 1) indices.push_back(0xFFFFFFFF);
    }
    m_gridIndexCount = static_cast<int>(indices.size());

    glGenVertexArrays(1, &m_gridVAO);
    glGenBuffers(1, &m_gridVBO);
    glGenBuffers(1, &m_gridEBO);

    glBindVertexArray(m_gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_gridEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
                 indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    return true;
}

// ── Accretion disk ring mesh ───────────────────────────────────────────────────
bool Renderer::initDisk() {
    // A ring drawn as a triangle strip between inner and outer radii
    const int   seg    = 128;
    const float rInner = 1.05f;
    const float rOuter = 2.2f;

    std::vector<float> verts;
    for (int i = 0; i <= seg; ++i) {
        float a = 2.f * 3.14159265f * i / seg;
        float cx = std::cos(a), cy = std::sin(a);
        verts.insert(verts.end(), {cx * rOuter, cy * rOuter, static_cast<float>(i) / seg});
        verts.insert(verts.end(), {cx * rInner, cy * rInner, static_cast<float>(i) / seg});
    }

    glGenVertexArrays(1, &m_diskVAO);
    glGenBuffers(1, &m_diskVBO);
    glBindVertexArray(m_diskVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_diskVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)(2*sizeof(float)));
    glBindVertexArray(0);
    return true;
}

void Renderer::resize(int w, int h) {
    m_fbWidth  = w;
    m_fbHeight = h;
    glViewport(0, 0, w, h);
}

// ── Main draw ──────────────────────────────────────────────────────────────────
void Renderer::draw(const Simulation& sim, const Camera& cam, float t) {
    glClearColor(0.015f, 0.010f, 0.035f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 1. Spacetime grid (drawn first, behind everything)
    if (showGrid)
        drawGrid(sim, cam);

    // 2. Trails
    if (showTrails) {
        for (const auto& b : sim.bodies())
            if (b.isAlive && b.trail.size() > 1)
                drawTrail(b, cam);
    }

    // 3. Accretion disks (drawn under the black hole body)
    for (const auto& b : sim.bodies()) {
        if (b.isAlive && b.isBlackHole())
            drawAccretionDisk(b, cam, t);
    }

    // 4. Bodies (planets / stars)
    for (const auto& b : sim.bodies()) {
        if (!b.isAlive) continue;
        if (b.isBlackHole())
            drawBlackHole(b, cam, t);
        else
            drawBody(b, cam);
    }
}

// ── Grid ───────────────────────────────────────────────────────────────────────
void Renderer::drawGrid(const Simulation& sim, const Camera& cam) {
    glUseProgram(m_gridProg);

    glm::mat4 vp = cam.viewProjection();
    glUniformMatrix4fv(glGetUniformLocation(m_gridProg, "u_VP"), 1, GL_FALSE, glm::value_ptr(vp));

    // Upload up to 8 masses for gravity-well vertex displacement
    const auto& bodies = sim.bodies();
    int n = 0;
    for (const auto& b : bodies) {
        if (!b.isAlive || n >= 8) continue;
        std::string base = "u_bodies[" + std::to_string(n) + "]";
        glUniform2f(glGetUniformLocation(m_gridProg, (base + ".pos").c_str()),
                    static_cast<float>(b.position.x), static_cast<float>(b.position.y));
        glUniform1f(glGetUniformLocation(m_gridProg, (base + ".mass").c_str()),
                    static_cast<float>(b.mass));
        ++n;
    }
    glUniform1i(glGetUniformLocation(m_gridProg, "u_bodyCount"), n);

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFFFFFF);
    glBindVertexArray(m_gridVAO);
    glDrawElements(GL_LINE_STRIP, m_gridIndexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glDisable(GL_PRIMITIVE_RESTART);
}

// ── Regular body (planet / star) ───────────────────────────────────────────────
void Renderer::drawBody(const Body& b, const Camera& cam) {
    glUseProgram(m_bodyProg);

    glm::mat4 vp    = cam.viewProjection();
    glm::vec2 pos   = glm::vec2(b.position);
    float     r     = b.radius;
    float     glow  = showGlow ? b.glowRadius : 1.0f;

    // Scale the quad to cover the glow halo
    glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(pos, 0.f));
    model           = glm::scale(model, glm::vec3(r * glow));

    glUniformMatrix4fv(glGetUniformLocation(m_bodyProg, "u_MVP"),    1, GL_FALSE, glm::value_ptr(vp * model));
    glUniform4fv(      glGetUniformLocation(m_bodyProg, "u_color"),  1, glm::value_ptr(b.color));
    glUniform1f(       glGetUniformLocation(m_bodyProg, "u_radius"), r);
    glUniform1f(       glGetUniformLocation(m_bodyProg, "u_glow"),   glow);
    glUniform1i(glGetUniformLocation(m_bodyProg, "u_isStar"),
               (b.isStar() || b.isNeutronStar()) ? 1 : 0);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// ── Black hole body ────────────────────────────────────────────────────────────
void Renderer::drawBlackHole(const Body& b, const Camera& cam, float t) {
    glUseProgram(m_blackholeProg);

    glm::mat4 vp  = cam.viewProjection();
    glm::vec2 pos = glm::vec2(b.position);

    // Scale quad to 3× radius to show lensing halo
    glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(pos, 0.f));
    model           = glm::scale(model, glm::vec3(b.radius * 3.f));

    glUniformMatrix4fv(glGetUniformLocation(m_blackholeProg, "u_MVP"),    1, GL_FALSE, glm::value_ptr(vp * model));
    glUniform1f(       glGetUniformLocation(m_blackholeProg, "u_radius"), b.radius);
    glUniform1f(       glGetUniformLocation(m_blackholeProg, "u_time"),   t);
    glUniform2f(       glGetUniformLocation(m_blackholeProg, "u_resolution"),
                       static_cast<float>(m_fbWidth), static_cast<float>(m_fbHeight));

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// ── Trail line strip ────────────────────────────────────────────────────────────
void Renderer::drawTrail(const Body& b, const Camera& cam) {
    const auto& pts = b.trail;
    size_t count = std::min(pts.size(), TRAIL_VBO_CAPACITY);
    if (count < 2) return;

    std::vector<float> data;
    data.reserve(count * 3);
    for (size_t i = 0; i < count; ++i) {
        float alpha = static_cast<float>(i) / (count - 1); // 0 = oldest (faded), 1 = newest
        data.push_back(pts[i].x);
        data.push_back(pts[i].y);
        data.push_back(alpha * 0.75f);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_trailVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());

    glUseProgram(m_trailProg);
    glm::mat4 vp = cam.viewProjection();
    glUniformMatrix4fv(glGetUniformLocation(m_trailProg, "u_VP"),    1, GL_FALSE, glm::value_ptr(vp));
    glUniform4fv(      glGetUniformLocation(m_trailProg, "u_color"), 1, glm::value_ptr(b.color));

    glBindVertexArray(m_trailVAO);
    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(count));
    glBindVertexArray(0);
}

// ── Accretion disk ─────────────────────────────────────────────────────────────
void Renderer::drawAccretionDisk(const Body& b, const Camera& cam, float t) {
    glUseProgram(m_accretionProg);

    glm::vec2 pos = glm::vec2(b.position);
    glm::mat4 vp  = cam.viewProjection();

    glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(pos, 0.f));
    model           = glm::scale(model, glm::vec3(b.radius));
    // Subtle rotation to animate the disk
    model           = glm::rotate(model, t * 0.35f, glm::vec3(0.f, 0.f, 1.f));

    glUniformMatrix4fv(glGetUniformLocation(m_accretionProg, "u_MVP"),  1, GL_FALSE, glm::value_ptr(vp * model));
    glUniform1f(       glGetUniformLocation(m_accretionProg, "u_time"), t);

    // Use additive blending for the glow of the disk
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBindVertexArray(m_diskVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (128 + 1) * 2);
    glBindVertexArray(0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

Renderer::~Renderer() {
    glDeleteProgram(m_bodyProg);
    glDeleteProgram(m_trailProg);
    glDeleteProgram(m_gridProg);
    glDeleteProgram(m_blackholeProg);
    glDeleteProgram(m_accretionProg);
    glDeleteVertexArrays(1, &m_quadVAO);
    glDeleteVertexArrays(1, &m_trailVAO);
    glDeleteVertexArrays(1, &m_gridVAO);
    glDeleteVertexArrays(1, &m_diskVAO);
    glDeleteBuffers(1, &m_quadVBO);
    glDeleteBuffers(1, &m_trailVBO);
    glDeleteBuffers(1, &m_gridVBO);
    glDeleteBuffers(1, &m_gridEBO);
    glDeleteBuffers(1, &m_diskVBO);
}
