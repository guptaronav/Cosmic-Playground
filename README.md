# Cosmic Playground

> An interactive gravity sandbox built with C++17 and OpenGL — simulate planets, stars, neutron stars, and black holes with real-time N-body physics, orbit trails, and a curved spacetime grid.

[![CI](https://github.com/guptaronav/Cosmic-Playground/actions/workflows/ci.yml/badge.svg)](https://github.com/guptaronav/Cosmic-Playground/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![OpenGL 4.1](https://img.shields.io/badge/OpenGL-4.1-green.svg)](https://www.opengl.org/)

---

## Table of Contents

1. [Screenshots](#screenshots)
2. [The Physics — Explained](#the-physics--explained)
3. [Body Types](#body-types)
4. [Preset Scenes](#preset-scenes)
5. [Controls & Navigation](#controls--navigation)
6. [Build Instructions](#build-instructions)
7. [Architecture Overview](#architecture-overview)
8. [Credits & Inspiration](#credits--inspiration)

---

## Screenshots

| Solar System | Binary Stars | Planet Near Black Hole |
|:---:|:---:|:---:|
| <img width="1000" height="700" alt="Solar System" src="https://github.com/user-attachments/assets/7a6d8a6a-2320-4745-83d8-e17d6fc20749" /> | <img width="1000" height="700" alt="Binary Stars" src="https://github.com/user-attachments/assets/f95102fb-3946-4762-9a3e-2c7920823c1e" /> | <img width="1000" height="700" alt="Planet Near Black Hole" src="https://github.com/user-attachments/assets/a8409c0a-1a31-4a69-b888-3967a350adda" /> |

---

## The Physics — Explained

This section walks through every equation used in the simulation, from first principles. If you want a visual walkthrough of these ideas, watch **kavan010's videos** — they are the direct inspiration for this project.

---

### 1. Newton's Law of Universal Gravitation

Every pair of massive bodies exerts an attractive force on each other proportional to their masses and inversely proportional to the square of the distance between them:

```
F = G * m₁ * m₂ / r²
```

In vector form (so we know which *direction* to push each body):

```
F⃗₁₂ = G * m₁ * m₂ * r̂₁₂ / r²
     = G * m₁ * m₂ * (r⃗₂ - r⃗₁) / |r⃗₂ - r⃗₁|³
```

where `r̂₁₂` is the unit vector pointing from body 1 toward body 2.

**Constants used in the simulation:**
- `G = 800.0` (simulation units — tunable in the Physics panel)
- Masses are in arbitrary "solar mass" units; 1.0 ≈ Earth-mass reference

Watch this: https://www.youtube.com/watch?v=kxkFaBG6a-A

---

### 2. Softened Gravity (Preventing the Singularity)

When two bodies get very close, `r → 0` and the force `F → ∞`. This causes the integrator to blow up. The standard fix is *gravitational softening* — we replace `r²` with `r² + ε²`:

```
F⃗ᵢⱼ = G * mᵢ * mⱼ * r⃗ᵢⱼ / (|r⃗ᵢⱼ|² + ε²)^(3/2)
```

The softening length `ε` sets a minimum effective distance below which the force stops growing. This keeps the simulation numerically stable without meaningfully changing the physics at normal distances.

**Why `(r² + ε²)^(3/2)` and not just `(r² + ε²)`?**

The `r³` in the denominator of the force vector comes from dividing the `r²` force magnitude by `r` to normalise the direction vector. The softened version replaces `r³ → (r² + ε²)^(3/2)` consistently throughout.

Default `ε = 0.15` world units (adjustable in the Physics panel).

---

### 3. Velocity Verlet Integration

The simulation advances position and velocity each frame using *Velocity Verlet*, which is more accurate than simple Euler integration for the same step size. It exactly conserves energy for certain systems and is *symplectic* — it respects the geometry of Hamiltonian mechanics.

**Step sequence per frame:**

```
1.  x(t+h) = x(t) + v(t)·h + ½·a(t)·h²       ← update positions
2.  a(t+h) = F[x(t+h)] / m                    ← recompute forces at new positions
3.  v(t+h) = v(t) + ½·(a(t) + a(t+h))·h      ← update velocities with average acceleration
```

**Why not plain Euler?**

Euler integration uses `v(t+h) = v(t) + a(t)·h` — it only looks at acceleration at the *start* of the step. For orbits, this systematically adds energy each frame, causing planets to spiral outward. Velocity Verlet averages the old and new acceleration, which dramatically reduces this drift.

---

### 4. Gravitational Potential and the Spacetime Grid

The *gravitational potential* at a point `x` due to all bodies is:

```
Φ(x) = -G · Σᵢ mᵢ / |x - xᵢ|
```

Potential is negative and becomes more negative (deeper) near massive objects. The **spacetime grid** visualises this: every grid vertex is displaced *downward* in the Y axis by an amount proportional to the local potential:

```
displacement = clamp( Φ(x) · scale,  -max_depth,  0 )
```

With perspective projection from above, these downward bowls look like genuine spacetime curvature wells — the classic *rubber sheet analogy*. The deeper the well, the brighter and more cyan the grid line colour.

> *Note: this is a visual analogy, not General Relativity. The rubber sheet shows Newtonian potential, not actual spacetime curvature. GR effects are approximated only in the black hole shader.*

---

### 5. Circular Orbital Velocity

When placing preset bodies on circular orbits, the required tangential velocity is derived by balancing gravitational attraction with the centripetal acceleration needed to maintain a circular path:

```
Gravitational force:  F = G·M·m / r²
Centripetal force:    F = m·v² / r

Setting equal and solving for v:

v = √(G·M / r)
```

This formula is used in every preset to set the initial velocity of each orbiting body, guaranteeing a stable circular orbit at `t = 0`.

---

### 6. The Schwarzschild Radius

A black hole's **event horizon** — the point of no return — occurs at the Schwarzschild radius:

```
rₛ = 2·G·M / c²
```

At real scales, `rₛ` for a stellar black hole is only a few kilometres. In the simulation we scale it visually so it is meaningful on screen:

```
rₛ_visual = 0.5 × black_hole_radius
```

Any body whose centre crosses `rₛ_visual` is absorbed: its mass is added to the black hole and it is removed from the simulation. The black hole grows slightly in visual radius each time.

---

### 7. Black Hole Shader — Lensing Approximation

Real gravitational lensing requires ray-marching geodesics in curved spacetime (as in kavan010's `black_hole` project). As an approximation, the shader renders four concentric visual layers:

| Layer | Physics analogy |
|-------|----------------|
| Solid black disk (`r < rₛ`) | Event horizon — no light escapes |
| Bright glowing ring (`r ≈ rₛ`) | Photon sphere — light orbits at 1.5 rₛ |
| Inverse-square brightness falloff | Gravitational lensing bends background light toward the hole |
| Dim outer halo | Scattered accretion light |

The photon sphere ring flickers with animated noise to simulate the unstable orbiting photons. The lensing halo uses swirl noise to mimic bent starlight.

---

### 8. N-body Complexity

Computing all pairwise forces is **O(N²)** per frame — every body interacts with every other body. For `N = 50` bodies this is 1,225 pair evaluations per frame, which is fast. For `N > 500` it becomes sluggish. Future work includes a **Barnes-Hut tree** (O(N log N)) or GPU compute shaders.

---

## Body Types

| Type | Visual | Physics notes |
|------|--------|--------------|
| **Planet** | Blue-tinted glowing circle | Standard gravitating mass. Moderate radius and mass. Starting point for most simulations. |
| **Star** | Yellow-white with bright corona | Same physics as a planet but heavier and larger. The shader adds a sharp stellar corona glow. Uses `u_isStar = 1` for the extra corona pass in the fragment shader. |
| **Neutron Star** | Pale blue, very small | Extremely dense — high mass packed into a tiny radius. Produces strong gravity relative to its visual size. |
| **Black Hole** | Dark disk with photon ring and accretion disk | Uses the dedicated `blackhole.frag` shader. Has an event horizon, photon sphere, lensing halo, and a separate rotating accretion disk rendered with additive blending. Absorbs any body that crosses the Schwarzschild radius. |

---

## Preset Scenes

### Solar System
Six planets (Mercury through Saturn) in stable circular orbits around a central star. Initial velocities are set exactly to `v = √(G·M/r)` so all orbits are circular at `t = 0`. A good starting scene to verify the integrator is energy-conserving.

### Binary Stars
Two equal-mass stars in a mutual circular orbit around their shared barycentre, plus a third body ("Tatooine") in a wide circumbinary orbit. Tests multi-body stability and the figure-8 mass symmetry. Inspired by real circumbinary exoplanet systems like Kepler-16b.

### Three-Body Chaos
The famous **Chenciner–Montgomery figure-eight solution** (2000): three equal-mass bodies chase each other around a figure-eight curve. This is an unstable periodic orbit — any small numerical perturbation causes it to diverge into chaos, which is exactly the point. A beautiful demonstration of sensitive dependence on initial conditions.

### Planet Near Black Hole
A planet in a slightly-below-circular eccentric orbit around a black hole. The orbit precesses over time due to the mass ratio. Eventually (depending on simulation speed) the planet crosses the event horizon and is absorbed, growing the black hole. Good scene for watching the spacetime grid deform dramatically.

### Accretion Disk
36 small particles arranged in three concentric rings around a supermassive black hole. Each particle is given exactly the circular orbital velocity for its radius. With the grid enabled, you can see the gravity well clearly and watch the differential rotation — inner particles orbit faster than outer ones (Keplerian rotation). This is how real accretion disks work.

---

## Controls & Navigation

### Camera

| Input | Action |
|-------|--------|
| **Right-click + drag** | Orbit — rotate yaw (horizontal) and pitch (vertical) |
| **Scroll wheel** | Zoom in / zoom out |
| **Middle-click + drag** | Pan — slide the camera target in the ground plane |
| **Ctrl + left-click + drag** | Pan (alternative) |
| **W A S D** / Arrow keys | Pan with keyboard |
| **Home** or **C** | Reset camera to default view |

**Pitch range:** nearly straight-down to nearly horizontal. The default is ~40° above the plane — a good angle to see the spacetime grid curvature.

**Zoom range:** 4 to 250 world units from the target.

### Simulation

| Key | Action |
|-----|--------|
| **Space** | Pause / Resume |
| **R** | Clear all bodies |
| **T** | Toggle orbit trails |
| **G** | Toggle spacetime grid |
| **Escape** | Quit |

### Interacting with Bodies

| Input | Action |
|-------|--------|
| **Left-click** | Select the nearest body (within 2.5 world units) |
| **Shift + Left-click** | Place a new body at the cursor (using the Add Body panel settings) |
| **Shift + Right-click + drag** | Aim launch velocity before placing |

### UI Panels

| Panel | Purpose |
|-------|---------|
| **Simulation** | Play/pause/reset, speed multiplier, trail/grid/absorption toggles |
| **Add Body** | Configure the pending body (name, type, mass, radius, colour) |
| **Selected Body** | Edit a body you have clicked on; remove it |
| **Presets** | Load one of the five built-in scenes |
| **Visuals** | Toggle trails, grid, and glow effects |
| **Physics** | Tune `G` (gravity constant) and `ε` (softening length) in real time |
| **Stats** | FPS, body count, total mechanical energy, centre-of-mass position |

---

## Build Instructions

### Prerequisites

| Tool | Version |
|------|---------|
| CMake | ≥ 3.20 |
| C++ compiler | C++17 (AppleClang 14+, GCC 9+, MSVC 2019+) |
| Git | any recent |

All libraries (GLFW, GLM, ImGui, GLAD) are fetched automatically at configure time.

### macOS

```bash
# Install CMake if not present
brew install cmake

git clone https://github.com/guptaronav/Cosmic-Playground.git
cd Cosmic-Playground

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.logicalcpu)

./build/CosmicPlayground
```

### Linux

```bash
sudo apt install cmake build-essential libgl1-mesa-dev

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

./build/CosmicPlayground
```

### Windows (MSVC)

```batch
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
.\build\Release\CosmicPlayground.exe
```

> **Note:** The first configure step downloads ~30 MB of dependencies (GLFW, GLM, ImGui, GLAD). A network connection is required on first build.

---

## Architecture Overview

```
Cosmic-Playground/
├── CMakeLists.txt          ← FetchContent build (GLFW, GLM, ImGui, GLAD)
├── .github/
│   └── workflows/
│       └── ci.yml          ← GitHub Actions: builds on push/PR
├── src/
│   ├── main.cpp            ← Entry point
│   ├── App.{h,cpp}         ← Window, GL context, main loop, input routing
│   ├── Simulation.{h,cpp}  ← N-body Velocity Verlet engine
│   ├── Body.{h,cpp}        ← Body data (type, mass, trail, colour)
│   ├── Renderer.{h,cpp}    ← All OpenGL draw calls, VAO/VBO management
│   ├── Camera.{h,cpp}      ← 3D orbit camera (yaw/pitch, pan, zoom)
│   ├── Input.{h,cpp}       ← GLFW input state machine
│   ├── UI.{h,cpp}          ← ImGui panels
│   └── Presets.{h,cpp}     ← Named scene factories
└── shaders/
    ├── body.{vert,frag}             ← Billboard glow circle (SDF)
    ├── trail.{vert,frag}            ← Fading line strip
    ├── grid.{vert,frag}             ← Gravity-well displaced XZ mesh
    ├── blackhole.{vert,frag}        ← Photon ring + lensing halo
    └── accretion_disk.{vert,frag}   ← Rotating plasma ring
```

### Key design decisions

- **Double-precision physics, float rendering** — simulation positions use `glm::dvec2` to avoid floating-point drift in long-running orbits; the renderer converts to `float` for the GPU.
- **Screen-space billboarding** — body quads are offset in *view space* before projection, so they always face the camera regardless of pitch/yaw.
- **Softened N-body** — all O(N²) pairwise forces use `(r²+ε²)^(3/2)` in the denominator.
- **Velocity Verlet** — stores both current and previous acceleration to average them, giving second-order accuracy without extra force evaluations.

---

## Credits & Inspiration

This project was directly inspired by **kavan010** and his outstanding OpenGL/C++ projects:

- **[kavan010/black_hole](https://github.com/kavan010/black_hole)** — C++17, GLFW, GLEW, GLM, GLSL black hole renderer with geodesic ray-marching and gravitational lensing. The shader architecture, black hole layering approach (event horizon → photon sphere → lensing halo), and the idea of a visual Schwarzschild radius all trace back to this project.
- **[kavan010/gravity_sim](https://github.com/kavan010/gravity_sim)** — Gravity simulation concept that seeded the idea for a more interactive and feature-rich sandbox.

kavan explains the math behind these topics incredibly well in his videos — if you want to understand the *real* physics (geodesics, GR lensing, Kerr metric) rather than the Newtonian approximations used here, his content is the place to start.

### Libraries

| Library | Purpose |
|---------|---------|
| [GLFW 3.4](https://www.glfw.org/) | Window creation and input |
| [GLAD v0.1](https://github.com/Dav1dde/glad) | OpenGL function loader |
| [GLM 1.0](https://github.com/g-truc/glm) | Vector and matrix math |
| [Dear ImGui 1.90](https://github.com/ocornut/imgui) | Immediate-mode UI |

---

## Roadmap

- [ ] Barnes-Hut O(N log N) approximation for large body counts
- [ ] Actual GR gravitational lensing via ray-marching (like kavan010/black_hole)
- [ ] 3D body trajectories (out-of-plane orbits)
- [ ] Save / load simulation state (JSON)
- [ ] Body merge animations on collision
- [ ] Time-series energy and angular momentum plots
- [ ] GPU compute shader N-body (eliminate CPU bottleneck)
- [ ] Body labels rendered in world space

---

## License

MIT — see [LICENSE](LICENSE).
