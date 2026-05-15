#include "Simulation.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <numeric>

Simulation::Simulation() = default;

// ──────────────────────────────────────────────────────────────────────────────
//  Main update: Velocity Verlet integration
// ──────────────────────────────────────────────────────────────────────────────
void Simulation::update(double dt) {
    if (m_cfg.paused || m_bodies.empty()) return;

    const double h = dt * m_cfg.timeScale;

    // 1. Position update using current velocity and acceleration
    for (auto& b : m_bodies) {
        if (!b.isAlive) continue;
        b.position += b.velocity * h + 0.5 * b.acceleration * h * h;
    }

    // 2. Save old accelerations, then recompute with new positions
    for (auto& b : m_bodies) {
        b.prevAccel    = b.acceleration;
        b.acceleration = {0.0, 0.0};
    }
    computeAccelerations();

    // 3. Velocity update using average of old and new accelerations
    for (auto& b : m_bodies) {
        if (!b.isAlive) continue;
        b.velocity += 0.5 * (b.prevAccel + b.acceleration) * h;
    }

    // 4. Trail recording
    if (m_cfg.trailsEnabled) {
        for (auto& b : m_bodies) {
            if (!b.isAlive) continue;
            b.trailTimer += static_cast<float>(dt);
            if (b.trailTimer >= Body::TRAIL_INTERVAL) {
                b.trailTimer = 0.f;
                b.pushTrailPoint();
            }
        }
    }

    // 5. Black hole absorption
    if (m_cfg.absorptionOn)
        checkAbsorption();

    pruneDeadBodies();
}

// ──────────────────────────────────────────────────────────────────────────────
//  Softened gravity between all pairs
// ──────────────────────────────────────────────────────────────────────────────
void Simulation::computeAccelerations() {
    const double eps2 = m_cfg.softeningEps * m_cfg.softeningEps;
    const double G    = m_cfg.G;
    const auto n      = m_bodies.size();

    for (size_t i = 0; i < n; ++i) {
        if (!m_bodies[i].isAlive) continue;
        for (size_t j = i + 1; j < n; ++j) {
            if (!m_bodies[j].isAlive) continue;

            glm::dvec2 r   = m_bodies[j].position - m_bodies[i].position;
            double     r2  = glm::dot(r, r) + eps2;
            double     r3  = r2 * std::sqrt(r2);           // (r²+ε²)^(3/2)
            double     gij = G / r3;

            // Newton's 3rd law: apply to both bodies simultaneously
            m_bodies[i].acceleration += gij * m_bodies[j].mass * r;
            m_bodies[j].acceleration -= gij * m_bodies[i].mass * r;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  Black holes absorb bodies that enter the visual event horizon
// ──────────────────────────────────────────────────────────────────────────────
void Simulation::checkAbsorption() {
    for (auto& bh : m_bodies) {
        if (!bh.isAlive || !bh.isBlackHole()) continue;
        const float rs = bh.schwarzschildRadius();

        for (auto& other : m_bodies) {
            if (!other.isAlive || other.id == bh.id) continue;
            glm::dvec2 delta  = other.position - bh.position;
            double     distSq = glm::dot(delta, delta);

            if (distSq < static_cast<double>(rs) * rs) {
                // Conserve mass (momentum is already handled by gravity)
                bh.mass    += other.mass;
                bh.radius  = std::max(bh.radius, other.radius * 0.4f);
                other.isAlive = false;
            }
        }
    }
}

void Simulation::pruneDeadBodies() {
    m_bodies.erase(
        std::remove_if(m_bodies.begin(), m_bodies.end(),
                       [](const Body& b) { return !b.isAlive; }),
        m_bodies.end());
}

// ──────────────────────────────────────────────────────────────────────────────
//  Body management
// ──────────────────────────────────────────────────────────────────────────────
uint32_t Simulation::addBody(Body body) {
    body.id = m_nextId++;
    m_bodies.push_back(std::move(body));
    return m_bodies.back().id;
}

void Simulation::removeBody(uint32_t id) {
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
                           [id](const Body& b) { return b.id == id; });
    if (it != m_bodies.end())
        it->isAlive = false;
}

void Simulation::clearBodies() {
    m_bodies.clear();
}

Body* Simulation::findBody(uint32_t id) {
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
                           [id](const Body& b) { return b.id == id; });
    return it != m_bodies.end() ? &(*it) : nullptr;
}

const Body* Simulation::findBody(uint32_t id) const {
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
                           [id](const Body& b) { return b.id == id; });
    return it != m_bodies.end() ? &(*it) : nullptr;
}

Body* Simulation::pickBody(glm::dvec2 worldPos, double maxDist) {
    Body*  nearest = nullptr;
    double minD2   = maxDist * maxDist;

    for (auto& b : m_bodies) {
        if (!b.isAlive) continue;
        glm::dvec2 d  = b.position - worldPos;
        double     d2 = glm::dot(d, d);
        if (d2 < minD2) { minD2 = d2; nearest = &b; }
    }
    return nearest;
}

// ──────────────────────────────────────────────────────────────────────────────
//  Analytics helpers
// ──────────────────────────────────────────────────────────────────────────────
double Simulation::totalEnergy() const {
    double KE = 0.0, PE = 0.0;
    for (const auto& b : m_bodies) {
        if (!b.isAlive) continue;
        KE += 0.5 * b.mass * glm::dot(b.velocity, b.velocity);
    }
    for (size_t i = 0; i < m_bodies.size(); ++i) {
        for (size_t j = i + 1; j < m_bodies.size(); ++j) {
            if (!m_bodies[i].isAlive || !m_bodies[j].isAlive) continue;
            glm::dvec2 r = m_bodies[j].position - m_bodies[i].position;
            double dist  = std::sqrt(glm::dot(r, r)) + 1e-6;
            PE -= m_cfg.G * m_bodies[i].mass * m_bodies[j].mass / dist;
        }
    }
    return KE + PE;
}

glm::dvec2 Simulation::centreOfMass() const {
    double     totalMass = 0.0;
    glm::dvec2 com       = {0.0, 0.0};
    for (const auto& b : m_bodies) {
        if (!b.isAlive) continue;
        com       += b.position * b.mass;
        totalMass += b.mass;
    }
    return totalMass > 0.0 ? com / totalMass : glm::dvec2{0.0, 0.0};
}
