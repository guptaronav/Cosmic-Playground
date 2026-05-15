#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <deque>
#include <cstdint>

// ──────────────────────────────────────────────────────────────────────────────
//  Body types that drive physics constants and rendering style
// ──────────────────────────────────────────────────────────────────────────────
enum class BodyType {
    Planet,
    Star,
    NeutronStar,
    BlackHole
};

inline const char* bodyTypeName(BodyType t) {
    switch (t) {
        case BodyType::Planet:      return "Planet";
        case BodyType::Star:        return "Star";
        case BodyType::NeutronStar: return "Neutron Star";
        case BodyType::BlackHole:   return "Black Hole";
    }
    return "Unknown";
}

// ──────────────────────────────────────────────────────────────────────────────
//  A single simulated body (planet, star, neutron star, or black hole)
//
//  Physics uses double precision; rendering converts to float.
//  Position is in simulation-world units (1 unit ≈ 1 AU for typical scenes).
// ──────────────────────────────────────────────────────────────────────────────
struct Body {
    uint32_t    id       = 0;
    std::string name     = "Body";
    BodyType    type     = BodyType::Planet;

    // ── Physics ───────────────────────────────────────────────────────────────
    double      mass     = 1.0;          // in simulation mass units
    float       radius   = 0.5f;         // visual radius (world units)

    glm::dvec2  position     = {0.0, 0.0};
    glm::dvec2  velocity     = {0.0, 0.0};
    glm::dvec2  acceleration = {0.0, 0.0};  // current step
    glm::dvec2  prevAccel    = {0.0, 0.0};  // previous step (Velocity Verlet)

    // ── Rendering ─────────────────────────────────────────────────────────────
    glm::vec4   color        = {1.f, 1.f, 1.f, 1.f};
    float       glowRadius   = 2.5f;    // halo multiplier relative to radius
    float       temperature  = 5778.f; // Kelvin (used for star colour tinting)

    // ── Trail ─────────────────────────────────────────────────────────────────
    static constexpr size_t MAX_TRAIL = 600;
    std::deque<glm::vec2>   trail;      // float positions are good enough here
    float                   trailTimer = 0.f;
    static constexpr float  TRAIL_INTERVAL = 0.05f; // seconds between trail points

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    bool isAlive = true;

    // ── Helpers ───────────────────────────────────────────────────────────────

    bool isBlackHole()   const { return type == BodyType::BlackHole;   }
    bool isStar()        const { return type == BodyType::Star;        }
    bool isNeutronStar() const { return type == BodyType::NeutronStar; }

    // Visual Schwarzschild radius scaled so it's visible in the sandbox.
    // Real rs = 2GM/c², but here we just use a mass-proportional scale.
    float schwarzschildRadius() const {
        return radius * 0.5f; // event horizon ≈ half the visual black-hole radius
    }

    // Add a trail point (called every TRAIL_INTERVAL seconds)
    void pushTrailPoint() {
        if (trail.size() >= MAX_TRAIL)
            trail.pop_front();
        trail.push_back(glm::vec2(position));
    }

    // Default colour for a body type
    static glm::vec4 defaultColor(BodyType t) {
        switch (t) {
            case BodyType::Planet:      return {0.30f, 0.60f, 1.00f, 1.f};
            case BodyType::Star:        return {1.00f, 0.85f, 0.40f, 1.f};
            case BodyType::NeutronStar: return {0.70f, 0.80f, 1.00f, 1.f};
            case BodyType::BlackHole:   return {0.05f, 0.05f, 0.08f, 1.f};
        }
        return {1.f, 1.f, 1.f, 1.f};
    }

    // Default radius for a body type
    static float defaultRadius(BodyType t) {
        switch (t) {
            case BodyType::Planet:      return 0.35f;
            case BodyType::Star:        return 0.80f;
            case BodyType::NeutronStar: return 0.20f;
            case BodyType::BlackHole:   return 1.20f;
        }
        return 0.35f;
    }
};
