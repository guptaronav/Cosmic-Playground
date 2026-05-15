#pragma once

#include "Simulation.h"
#include <string>
#include <vector>
#include <functional>

struct Preset {
    std::string name;
    std::string description;
    std::function<void(Simulation&)> load;
};

// ──────────────────────────────────────────────────────────────────────────────
//  Built-in preset scenes.  Each preset clears the simulation and populates it
//  with a handcrafted configuration.
// ──────────────────────────────────────────────────────────────────────────────
class Presets {
public:
    Presets();

    const std::vector<Preset>& list() const { return m_presets; }

    // Apply preset by index to the simulation (clears existing bodies first)
    void apply(size_t index, Simulation& sim) const;

private:
    std::vector<Preset> m_presets;

    static void loadSolarSystem(Simulation& sim);
    static void loadBinaryStars(Simulation& sim);
    static void loadThreeBody(Simulation& sim);
    static void loadPlanetNearBlackHole(Simulation& sim);
    static void loadAccretionDisk(Simulation& sim);
};
