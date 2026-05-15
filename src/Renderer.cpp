#include "Renderer.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <vector>

// ──────────────────────────────────────────────────────────────────────────────
//  Shader helpers
// ──────────────────────────────────────────────────────────────────────────────
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
        char buf[1024]; glGetShaderInfoLog(id, sizeof(buf), nullptr, buf);
        std::cerr << "[Renderer] Shader compile error:\n" << buf << "\n";
        glDeleteShader(id); return 0;
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
        char buf[1024]; glGetProgramInfoLog(prog, sizeof(buf), nullptr, buf);
        std::cerr << "[Renderer] Program link error:\n" << buf << "\n";
        glDeleteProgram(prog); return 0;
    }
    glDeleteShader(vert); glDeleteShader(frag);
    return prog;
}

GLuint Renderer::loadProgram(const std::string& vp, const std::string& fp) {
    std::string vs = readFile(vp), fs = readFile(fp);
    if (vs.empty() || fs.empty()) return 0;
    GLuint v = compileShader(GL_VERTEX_SHADER,   vs.c_str());
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs.c_str());
    if (!v || !f) return 0;
    return linkProgram(v, f);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Init
// ──────────────────────────────────────────────────────────────────────────────
bool Renderer::init() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    m_bodyProg      = loadProgram("shaders/body.vert",           "shaders/body.frag");
    m_trailProg     = loadProgram("shaders/trail.vert",          "shaders/trail.frag");
    m_gridProg      = loadProgram("shaders/grid.vert",           "shaders/grid.frag");
    m_blackholeProg = loadProgram("shaders/blackhole.vert",      "shaders/blackhole.frag");
    m_accretionProg = loadProgram("shaders/accretion_disk.vert", "shaders/accretion_disk.frag");

    if (!m_bodyProg || !m_trailProg || !m_gridProg ||
        !m_blackholeProg || !m_accretionProg) return false;

    return initQuad() && initTrail() && initGrid() && initDisk();
}

// ──────────────────────────────────────────────────────────────────────────────
//  Unit quad (triangle strip, BL/BR/TL/TR)
// ──────────────────────────────────────────────────────────────────────────────
bool Renderer::initQuad() {
    float verts[] = {
        -1.f,-1.f,  0.f,0.f,
         1.f,-1.f,  1.f,0.f,
        -1.f, 1.f,  0.f,1.f,
         1.f, 1.f,  1.f,1.f,
    };
    glGenVertexArrays(1, &m_quadVAO); glGenBuffers(1, &m_quadVBO);
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

// ──────────────────────────────────────────────────────────────────────────────
//  Trail VBO -- dynamic, (x, z, alpha) per point
// ──────────────────────────────────────────────────────────────────────────────
bool Renderer::initTrail() {
    glGenVertexArrays(1, &m_trailVAO); glGenBuffers(1, &m_trailVBO);
    glBindVertexArray(m_trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_trailVBO);
    glBufferData(GL_ARRAY_BUFFER, TRAIL_VBO_CAPACITY * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)(2*sizeof(float)));
    glBindVertexArray(0);
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
//  Spacetime grid -- flat XZ mesh.
//  Vertices stored as vec2 (x,z); interpreted as world XZ in the shader.
//  Grid lines at ±GRID_EXTENT with GRID_LINES divisions per axis.
// ──────────────────────────────────────────────────────────────────────────────
bool Renderer::initGrid() {
    const int   N   = GRID_LINES + 1;
    const float ext = GRID_EXTENT;

    std::vector<float> verts;
    verts.reserve(N * N * 2);
    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i) {
            verts.push_back(-ext + 2.f * ext * i / (N - 1));
            verts.push_back(-ext + 2.f * ext * j / (N - 1));
        }
    }

    // Index buffer: horizontal + vertical line strips, separated by restart token
    std::vector<uint32_t> idx;
    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i) idx.push_back(j * N + i);
        if (j < N - 1) idx.push_back(0xFFFFFFFF);
    }
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) idx.push_back(j * N + i);
        if (i < N - 1) idx.push_back(0xFFFFFFFF);
    }
    m_gridIndexCount = static_cast<int>(idx.size());

    glGenVertexArrays(1, &m_gridVAO);
    glGenBuffers(1, &m_gridVBO);
    glGenBuffers(1, &m_gridEBO);
    glBindVertexArray(m_gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_gridEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(uint32_t), idx.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
//  Accretion disk -- triangle strip ring in local XY
// ──────────────────────────────────────────────────────────────────────────────
bool Renderer::initDisk() {
    const int   seg    = 128;
    const float rInner = 1.05f;
    const float rOuter = 2.20f;
    std::vector<float> verts;
    for (int i = 0; i <= seg; ++i) {
        float a  = 2.f * 3.14159265f * i / seg;
        float cx = std::cos(a), cy = std::sin(a);
        float t  = static_cast<float>(i) / seg;
        verts.insert(verts.end(), {cx*rOuter, cy*rOuter, t});
        verts.insert(verts.end(), {cx*rInner, cy*rInner, t});
    }
    glGenVertexArrays(1, &m_diskVAO); glGenBuffers(1, &m_diskVBO);
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
    m_fbWidth = w; m_fbHeight = h;
    glViewport(0, 0, w, h);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Main draw
// ──────────────────────────────────────────────────────────────────────────────
void Renderer::draw(const Simulation& sim, const Camera& cam, float t) {
    glClearColor(0.012f, 0.008f, 0.025f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Pre-compute matrices shared across draw calls
    glm::mat4 V  = cam.viewMatrix();
    glm::mat4 P  = cam.projectionMatrix();
    glm::mat4 VP = P * V;
    glm::vec3 eye = cam.eyePosition();

    // 1. Spacetime grid (behind everything)
    if (showGrid)
        drawGrid(sim, VP, eye);

    // 2. Orbit trails
    if (showTrails)
        for (const auto& b : sim.bodies())
            if (b.isAlive && b.trail.size() > 1)
                drawTrail(b, VP);

    // 3. Accretion disks (under the black hole billboard)
    for (const auto& b : sim.bodies())
        if (b.isAlive && b.isBlackHole())
            drawAccretionDisk(b, VP, t);

    // 4. Bodies
    for (const auto& b : sim.bodies()) {
        if (!b.isAlive) continue;
        if (b.isBlackHole())
            drawBlackHole(b, V, P, t);
        else
            drawBody(b, V, P);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  Grid
// ──────────────────────────────────────────────────────────────────────────────
void Renderer::drawGrid(const Simulation& sim, const glm::mat4& VP, const glm::vec3& eye) {
    glUseProgram(m_gridProg);
    glUniformMatrix4fv(glGetUniformLocation(m_gridProg, "u_VP"),     1, GL_FALSE, glm::value_ptr(VP));
    glUniform3fv(      glGetUniformLocation(m_gridProg, "u_eyePos"), 1, glm::value_ptr(eye));

    int n = 0;
    for (const auto& b : sim.bodies()) {
        if (!b.isAlive || n >= 8) continue;
        std::string base = "u_bodies[" + std::to_string(n) + "]";
        glUniform2f(glGetUniformLocation(m_gridProg, (base + ".pos" ).c_str()),
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

// ──────────────────────────────────────────────────────────────────────────────
//  Planet / star billboard
// ──────────────────────────────────────────────────────────────────────────────
void Renderer::drawBody(const Body& b, const glm::mat4& V, const glm::mat4& P) {
    glUseProgram(m_bodyProg);

    glm::vec3 wp  = glm::vec3(b.position.x, 0.f, b.position.y);
    float     sc  = b.radius * (showGlow ? b.glowRadius : 1.f);

    glUniformMatrix4fv(glGetUniformLocation(m_bodyProg, "u_V"),        1, GL_FALSE, glm::value_ptr(V));
    glUniformMatrix4fv(glGetUniformLocation(m_bodyProg, "u_P"),        1, GL_FALSE, glm::value_ptr(P));
    glUniform3fv(      glGetUniformLocation(m_bodyProg, "u_worldPos"), 1, glm::value_ptr(wp));
    glUniform1f(       glGetUniformLocation(m_bodyProg, "u_scale"),    sc);
    glUniform4fv(      glGetUniformLocation(m_bodyProg, "u_color"),    1, glm::value_ptr(b.color));
    glUniform1f(       glGetUniformLocation(m_bodyProg, "u_radius"),   b.radius);
    glUniform1f(       glGetUniformLocation(m_bodyProg, "u_glow"),     b.glowRadius);
    glUniform1i(       glGetUniformLocation(m_bodyProg, "u_isStar"),   (b.isStar() || b.isNeutronStar()) ? 1 : 0);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Black hole billboard
// ──────────────────────────────────────────────────────────────────────────────
void Renderer::drawBlackHole(const Body& b, const glm::mat4& V, const glm::mat4& P, float t) {
    glUseProgram(m_blackholeProg);

    glm::vec3 wp = glm::vec3(b.position.x, 0.f, b.position.y);
    float sc     = b.radius * 3.f;

    glUniformMatrix4fv(glGetUniformLocation(m_blackholeProg, "u_V"),        1, GL_FALSE, glm::value_ptr(V));
    glUniformMatrix4fv(glGetUniformLocation(m_blackholeProg, "u_P"),        1, GL_FALSE, glm::value_ptr(P));
    glUniform3fv(      glGetUniformLocation(m_blackholeProg, "u_worldPos"), 1, glm::value_ptr(wp));
    glUniform1f(       glGetUniformLocation(m_blackholeProg, "u_scale"),    sc);
    glUniform1f(       glGetUniformLocation(m_blackholeProg, "u_radius"),   b.radius);
    glUniform1f(       glGetUniformLocation(m_blackholeProg, "u_time"),     t);
    glUniform2f(       glGetUniformLocation(m_blackholeProg, "u_resolution"),
                       static_cast<float>(m_fbWidth), static_cast<float>(m_fbHeight));

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Orbit trail
// ──────────────────────────────────────────────────────────────────────────────
void Renderer::drawTrail(const Body& b, const glm::mat4& VP) {
    const auto& pts   = b.trail;
    size_t      count = std::min(pts.size(), TRAIL_VBO_CAPACITY);
    if (count < 2) return;

    std::vector<float> data;
    data.reserve(count * 3);
    for (size_t i = 0; i < count; ++i) {
        float alpha = static_cast<float>(i) / (count - 1);
        data.push_back(pts[i].x);
        data.push_back(pts[i].y);
        data.push_back(alpha * 0.75f);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_trailVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());

    glUseProgram(m_trailProg);
    glUniformMatrix4fv(glGetUniformLocation(m_trailProg, "u_VP"),    1, GL_FALSE, glm::value_ptr(VP));
    glUniform4fv(      glGetUniformLocation(m_trailProg, "u_color"), 1, glm::value_ptr(b.color));

    glBindVertexArray(m_trailVAO);
    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(count));
    glBindVertexArray(0);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Accretion disk -- lies flat in XZ via model-matrix X-rotation
// ──────────────────────────────────────────────────────────────────────────────
void Renderer::drawAccretionDisk(const Body& b, const glm::mat4& VP, float t) {
    glUseProgram(m_accretionProg);

    // Translate to body world position (XZ), rotate local XY to world XZ, scale
    glm::vec3 wp    = glm::vec3(b.position.x, 0.2f, b.position.y); // slight Y lift
    glm::mat4 model = glm::translate(glm::mat4(1.f), wp);
    model *= glm::rotate(glm::mat4(1.f), glm::half_pi<float>(), glm::vec3(1.f,0.f,0.f));
    model *= glm::scale(glm::mat4(1.f), glm::vec3(b.radius));
    model *= glm::rotate(glm::mat4(1.f), t * 0.35f, glm::vec3(0.f,0.f,1.f));

    glm::mat4 MVP = VP * model;
    glUniformMatrix4fv(glGetUniformLocation(m_accretionProg, "u_MVP"),  1, GL_FALSE, glm::value_ptr(MVP));
    glUniform1f(       glGetUniformLocation(m_accretionProg, "u_time"), t);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive for plasma glow
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
