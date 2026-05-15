#pragma once

#include "Body.h"
#include <vector>
#include <functional>
#include <cstdint>

// ──────────────────────────────────────────────────────────────────────────────
//  Physics configuration tweakable at runtime via the UI
// ──────────────────────────────────────────────────────────────────────────────
struct SimConfig {
    double G              = 800.0;   // gravitational constant (simulation units)
    double softeningEps   = 0.15;    // softening length to prevent singularity
    double timeScale      = 1.0;     // multiplier on real dt
    bool   paused         = false;
    bool   trailsEnabled  = true;
    bool   gridEnabled    = true;
    bool   absorptionOn   = true;    // black holes eat nearby bodies
};

// ──────────────────────────────────────────────────────────────────────────────
//  N-body gravity simulation using Velocity Verlet integration.
//
//  Softened gravity force between bodies i and j:
//    F⃗ = G·mᵢ·mⱼ · r⃗ᵢⱼ / (|r⃗ᵢⱼ|² + ε²)^(3/2)
//
//  Velocity Verlet:
//    x(t+h) = x(t) + v(t)·h + ½·a(t)·h²
//    a(t+h) = F(t+h)/m
//    v(t+h) = v(t) + ½·(a(t) + a(t+h))·h
// ──────────────────────────────────────────────────────────────────────────────
class Simulation {
public:
    explicit Simulation();

    // Step the simulation forward by dt seconds of real time
    void update(double dt);

    // Body management
    uint32_t addBody(Body body);
    void     removeBody(uint32_t id);
    void     clearBodies();

    Body*       findBody(uint32_t id);
    const Body* findBody(uint32_t id) const;

    // Selects the body nearest to world-space point p (within maxDist)
    // Returns nullptr if nothing nearby.
    Body* pickBody(glm::dvec2 worldPos, double maxDist = 1.5);

    const std::vector<Body>& bodies() const { return m_bodies; }
    std::vector<Body>&       bodies()       { return m_bodies; }

    SimConfig& config()             { return m_cfg; }
    const SimConfig& config() const { return m_cfg; }

    // Total kinetic + potential energy (for stats display)
    double totalEnergy() const;

    // Centre-of-mass position (for camera target)
    glm::dvec2 centreOfMass() const;

private:
    std::vector<Body> m_bodies;
    SimConfig         m_cfg;
    uint32_t          m_nextId = 1;

    // Recompute accelerations for all alive bodies
    void computeAccelerations();

    // Prune dead bodies that were flagged isAlive=false
    void pruneDeadBodies();

    // Handle black hole absorption
    void checkAbsorption();
};
