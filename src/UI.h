#pragma once

#include "Simulation.h"
#include "Renderer.h"
#include "Camera.h"
#include "Presets.h"
#include <cstdint>

struct GLFWwindow;

// ──────────────────────────────────────────────────────────────────────────────
//  All ImGui panels for Cosmic Playground.
//
//  UI is a passive presenter: it reads simulation state, and mutates it only
//  through the public API (Simulation::config(), addBody(), etc.).
// ──────────────────────────────────────────────────────────────────────────────
class UI {
public:
    // Initialise ImGui contexts; call once after the GL context is current
    void init(GLFWwindow* window);
    void shutdown();

    // Begin/end per-frame ImGui render
    void beginFrame();
    void endFrame();

    // Draw all panels; returns true if any ImGui widget captured this frame's input
    bool draw(Simulation& sim, Renderer& renderer, Camera& cam,
              const Presets& presets, float fps, float simTime);

    // Body selected in the viewport (nullptr = none)
    uint32_t selectedBodyId = 0;

    // Pending new-body configuration (filled via the Add Body panel)
    Body      pendingBody;
    bool      addBodyMode    = false;
    bool      velocityDrag   = false;  // right-drag to set launch velocity
    glm::vec2 launchStart    = {0.f, 0.f};

private:
    void panelSimControls(Simulation& sim, float fps, float simTime);
    void panelAddBody(Simulation& sim, Camera& cam);
    void panelSelectedBody(Simulation& sim);
    void panelPresets(Simulation& sim, const Presets& presets, Camera& cam);
    void panelVisuals(Renderer& renderer);
    void panelPhysics(Simulation& sim);
    void panelStats(const Simulation& sim, float fps);
};
