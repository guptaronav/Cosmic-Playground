#include "Presets.h"
#include <cmath>

// ──────────────────────────────────────────────────────────────────────────────
//  Velocity for a circular orbit of a test mass around a central body.
//    v = sqrt(G * M / r)
// ──────────────────────────────────────────────────────────────────────────────
static glm::dvec2 orbitalVelocity(double G, double centralMass,
                                   glm::dvec2 pos, glm::dvec2 centre) {
    glm::dvec2 r = pos - centre;
    double     d = glm::length(r);
    if (d < 1e-9) return {0.0, 0.0};
    double     v = std::sqrt(G * centralMass / d);
    // Perpendicular to r in 2-D: rotate 90° counter-clockwise
    return {-r.y / d * v, r.x / d * v};
}

// ──────────────────────────────────────────────────────────────────────────────

Presets::Presets() {
    m_presets.push_back({"Solar System",
        "Sun with several orbiting planets",
        [](Simulation& s){ loadSolarSystem(s); }});

    m_presets.push_back({"Binary Stars",
        "Two equal-mass stars in mutual orbit with planets",
        [](Simulation& s){ loadBinaryStars(s); }});

    m_presets.push_back({"Three-Body Chaos",
        "Three equal-mass bodies in an unstable figure-eight-like orbit",
        [](Simulation& s){ loadThreeBody(s); }});

    m_presets.push_back({"Planet Near Black Hole",
        "A planet spiralling around a black hole",
        [](Simulation& s){ loadPlanetNearBlackHole(s); }});

    m_presets.push_back({"Accretion Disk",
        "Many small bodies orbiting a central black hole",
        [](Simulation& s){ loadAccretionDisk(s); }});
}

void Presets::apply(size_t index, Simulation& sim) const {
    if (index >= m_presets.size()) return;
    sim.clearBodies();
    m_presets[index].load(sim);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Preset implementations
// ──────────────────────────────────────────────────────────────────────────────

void Presets::loadSolarSystem(Simulation& sim) {
    const double G = sim.config().G;

    // Sun
    Body sun;
    sun.name   = "Sun";
    sun.type   = BodyType::Star;
    sun.mass   = 1000.0;
    sun.radius = 1.2f;
    sun.color  = {1.f, 0.90f, 0.30f, 1.f};
    sun.position = {0.0, 0.0};
    sim.addBody(sun);

    // Planet helper
    auto addPlanet = [&](const char* name, double dist, double mass,
                         float r, glm::vec4 col) {
        Body p;
        p.name     = name;
        p.type     = BodyType::Planet;
        p.mass     = mass;
        p.radius   = r;
        p.color    = col;
        p.position = {dist, 0.0};
        p.velocity = orbitalVelocity(G, sun.mass, p.position, {0,0});
        sim.addBody(p);
    };

    addPlanet("Mercury", 2.5,  0.5f,  0.12f, {0.72f, 0.63f, 0.55f, 1.f});
    addPlanet("Venus",   4.0,  1.2f,  0.22f, {0.92f, 0.80f, 0.45f, 1.f});
    addPlanet("Earth",   6.0,  1.0f,  0.24f, {0.30f, 0.55f, 0.90f, 1.f});
    addPlanet("Mars",    8.5,  0.6f,  0.18f, {0.80f, 0.38f, 0.20f, 1.f});
    addPlanet("Jupiter", 13.0, 8.0f,  0.55f, {0.85f, 0.72f, 0.55f, 1.f});
    addPlanet("Saturn",  18.0, 5.0f,  0.48f, {0.90f, 0.80f, 0.60f, 1.f});
}

void Presets::loadBinaryStars(Simulation& sim) {
    const double G   = sim.config().G;
    const double sep = 5.0;
    const double M   = 400.0;

    // Each star orbits the common barycentre at sep/2
    double v = std::sqrt(G * M / (2.0 * sep));

    Body s1;
    s1.name = "Star A"; s1.type = BodyType::Star;
    s1.mass = M; s1.radius = 0.9f;
    s1.color    = {1.f, 0.80f, 0.30f, 1.f};
    s1.position = {-sep / 2.0, 0.0};
    s1.velocity = {0.0, -v};
    sim.addBody(s1);

    Body s2;
    s2.name = "Star B"; s2.type = BodyType::Star;
    s2.mass = M; s2.radius = 0.9f;
    s2.color    = {1.f, 0.60f, 0.20f, 1.f};
    s2.position = {sep / 2.0, 0.0};
    s2.velocity = {0.0, v};
    sim.addBody(s2);

    // A planet in a wide circumbinary orbit
    Body p;
    p.name = "Tatooine"; p.type = BodyType::Planet;
    p.mass = 0.5; p.radius = 0.22f;
    p.color    = {0.60f, 0.80f, 0.55f, 1.f};
    p.position = {0.0, 14.0};
    p.velocity = {-std::sqrt(G * 2.0 * M / 14.0), 0.0};
    sim.addBody(p);
}

void Presets::loadThreeBody(Simulation& sim) {
    // Figure-eight solution (Chenciner & Montgomery 2000)
    // Normalised positions and velocities, scaled to sim units
    const double scale = 6.0;
    const double vscale = 3.5;
    const double M = 200.0;
    sim.config().G = 1200.0;

    auto add = [&](glm::dvec2 pos, glm::dvec2 vel, glm::vec4 col, const char* name) {
        Body b;
        b.name = name; b.type = BodyType::Star;
        b.mass = M; b.radius = 0.5f; b.color = col;
        b.position = pos * scale;
        b.velocity = vel * vscale;
        sim.addBody(b);
    };

    add({ 0.97000436, -0.24308753}, { 0.93240737/2.0,  0.86473146/2.0}, {1.f,0.5f,0.3f,1.f}, "Alpha");
    add({-0.97000436,  0.24308753}, { 0.93240737/2.0,  0.86473146/2.0}, {0.3f,0.7f,1.f,1.f}, "Beta");
    add({0.0,          0.0       }, {-0.93240737,      -0.86473146    }, {0.5f,1.f,0.5f,1.f}, "Gamma");
}

void Presets::loadPlanetNearBlackHole(Simulation& sim) {
    const double G = sim.config().G;

    Body bh;
    bh.name   = "Singularity";
    bh.type   = BodyType::BlackHole;
    bh.mass   = 2000.0;
    bh.radius = 1.5f;
    bh.color  = {0.03f, 0.03f, 0.05f, 1.f};
    bh.position = {0.0, 0.0};
    sim.addBody(bh);

    // Planet in an eccentric orbit
    Body p;
    p.name = "Doomed World"; p.type = BodyType::Planet;
    p.mass = 1.0; p.radius = 0.28f;
    p.color = {0.35f, 0.65f, 1.f, 1.f};
    p.position = {9.0, 0.0};
    p.velocity = orbitalVelocity(G, bh.mass, p.position, {0,0}) * 0.88;
    sim.addBody(p);

    // A second body to show lensing interaction
    Body p2;
    p2.name = "Observer"; p2.type = BodyType::Planet;
    p2.mass = 0.5; p2.radius = 0.18f;
    p2.color = {0.9f, 0.7f, 0.3f, 1.f};
    p2.position = {-12.0, 2.0};
    p2.velocity = orbitalVelocity(G, bh.mass, p2.position, {0,0}) * 1.05;
    sim.addBody(p2);
}

void Presets::loadAccretionDisk(Simulation& sim) {
    const double G = sim.config().G;

    Body bh;
    bh.name   = "Colossus";
    bh.type   = BodyType::BlackHole;
    bh.mass   = 5000.0;
    bh.radius = 1.8f;
    bh.color  = {0.02f, 0.02f, 0.04f, 1.f};
    bh.position = {0.0, 0.0};
    sim.addBody(bh);

    // Ring of small bodies at varying radii – they form the accretion disk
    const int   rings    = 3;
    const int   perRing  = 12;
    const float baseR[]  = {4.f, 6.f, 8.5f};
    const glm::vec4 diskColors[] = {
        {1.f, 0.6f, 0.15f, 1.f},
        {1.f, 0.45f, 0.10f, 1.f},
        {0.9f, 0.35f, 0.08f, 1.f},
    };

    for (int ring = 0; ring < rings; ++ring) {
        double r = baseR[ring];
        for (int i = 0; i < perRing; ++i) {
            double angle = 2.0 * 3.14159265358979 * i / perRing;
            Body p;
            p.name   = "Particle";
            p.type   = BodyType::Planet;
            p.mass   = 0.1;
            p.radius = 0.08f;
            p.color  = diskColors[ring];
            p.position = {r * std::cos(angle), r * std::sin(angle)};
            p.velocity = orbitalVelocity(G, bh.mass, p.position, {0,0});
            sim.addBody(p);
        }
    }
}
