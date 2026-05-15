# Cosmic Playground

**An interactive gravity sandbox with real-time N-body physics, black holes, orbit trails, and spacetime curvature visuals.**

Built with C++17, OpenGL 4.1, GLSL, GLFW, GLM, and ImGui — focused on being both physically meaningful and visually impressive.

---

## Screenshots

> _(Add screenshots here after running the application)_

| Solar System | Black Hole + Accretion Disk | Three-Body Chaos |
|---|---|---|
| _screenshot_ | _screenshot_ | _screenshot_ |

---

## Features

- **Real-time N-body gravity simulation** using softened Newtonian gravity and Velocity Verlet integration
- **Five body types**: Planet, Star, Neutron Star, Black Hole — each with distinct rendering
- **Interactive body placement**: Shift+Click to place bodies, right-click drag to aim velocity
- **Orbit trails** that fade elegantly behind every body
- **Spacetime grid** that warps and deforms near massive objects in real time
- **Black hole rendering**: event horizon, photon sphere glow ring, lensing halo, animated accretion disk
- **Body absorption**: objects crossing the Schwarzschild radius are consumed by black holes
- **Five preset scenes** ready to load instantly
- **ImGui control panels**: add/edit bodies, tune physics constants, toggle visuals
- **FPS counter, energy display, centre-of-mass tracker**

---

## Build Instructions

### Prerequisites

| Tool | Minimum version |
|------|-----------------|
| CMake | 3.20 |
| C++ compiler | C++17 (GCC 9+, Clang 10+, MSVC 2019+) |
| Git | (for FetchContent to download deps) |

All other dependencies (GLFW, GLM, ImGui, GLAD) are fetched automatically by CMake.

### macOS

```bash
# Install CMake if needed
brew install cmake

git clone https://github.com/yourusername/Cosmic-Playground.git
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
build\Release\CosmicPlayground.exe
```

---

## Controls

| Input | Action |
|-------|--------|
| **Space** | Pause / Resume simulation |
| **R** | Clear all bodies |
| **T** | Toggle orbit trails |
| **G** | Toggle spacetime grid |
| **W A S D** / Arrow keys | Pan camera |
| **Scroll wheel** | Zoom in / out |
| **Left click** | Select body |
| **Shift + Left click** | Place new body at cursor |
| **Right click + drag** | Set launch velocity for next placed body |
| **Middle mouse drag** | Pan camera |
| **Escape** | Quit |

---

## Preset Scenes

| Scene | Description |
|-------|-------------|
| Solar System | Sun with six planets in stable circular orbits |
| Binary Stars | Two equal-mass stars co-orbiting; one circumbinary planet |
| Three-Body Chaos | Chaotic figure-eight starting configuration |
| Planet Near Black Hole | Doomed world in an inward-spiralling eccentric orbit |
| Accretion Disk | Dense ring of test particles orbiting a supermassive black hole |

---

## Architecture Overview

```
CosmicPlayground/
├── CMakeLists.txt          — Build system (FetchContent for all deps)
├── src/
│   ├── main.cpp            — Entry point
│   ├── App.{h,cpp}         — Window, GL context, main loop, input routing
│   ├── Simulation.{h,cpp}  — N-body physics engine
│   ├── Body.{h,cpp}        — Body data structure and helpers
│   ├── Renderer.{h,cpp}    — All OpenGL drawing (shaders, VAOs, draw calls)
│   ├── Camera.{h,cpp}      — 2D orthographic camera (pan + zoom)
│   ├── Input.{h,cpp}       — GLFW input state machine
│   ├── UI.{h,cpp}          — ImGui panels
│   └── Presets.{h,cpp}     — Named preset scene factories
└── shaders/
    ├── body.{vert,frag}            — Planet / star SDF glow rendering
    ├── trail.{vert,frag}           — Fading orbit trail line strips
    ├── grid.{vert,frag}            — Gravity-warped spacetime grid
    ├── blackhole.{vert,frag}       — Event horizon + photon ring + lensing halo
    └── accretion_disk.{vert,frag}  — Animated hot-plasma ring
```

---

## Physics Implementation

### Gravitational Force (Softened)

Standard Newtonian gravity is modified with a softening parameter ε to prevent
numerical blow-up when two bodies pass very close:

```
F⃗ᵢⱼ = G · mᵢ · mⱼ · r⃗ᵢⱼ / (|r⃗ᵢⱼ|² + ε²)^(3/2)
```

The softening length `ε` is configurable in the Physics panel (default 0.15 world units).

### Integration: Velocity Verlet

More accurate than simple Euler integration for the same step size:

```
x(t+h) = x(t) + v(t)·h + ½·a(t)·h²
a(t+h) = ΣF(t+h) / m          ← recompute forces at new positions
v(t+h) = v(t) + ½·(a(t) + a(t+h))·h
```

### Black Hole Absorption

When any body's centre enters the visual Schwarzschild radius of a black hole,
the body is absorbed: its mass is added to the black hole and it is removed
from the simulation. The Schwarzschild radius displayed is:

```
rs_visual = 0.5 × black_hole_radius
```

(The true `rs = 2GM/c²` is impractical at simulation scales; this is a
visually calibrated approximation.)

### Spacetime Grid Warping

The grid vertex shader computes the Newtonian gravitational potential at each
vertex position and applies a vertical displacement proportional to Φ:

```
Φ(x) = Σᵢ -G·mᵢ / |x − xᵢ|
```

This creates the classic "rubber sheet" visualization of curved spacetime.

---

## Limitations & Known Issues

- Physics uses **2D Newtonian gravity** — no relativistic effects (GR lensing is approximated in the shader only)
- No adaptive time-stepping: very close fast-moving bodies may tunnel through each other
- Grid displacement is computed on the CPU side only through uniforms; bodies > 8 are not sent to the grid shader
- No texture mapping or 3D projection (architecture supports extension to 3D)

---

## Roadmap / Future Features

- [ ] Barnes-Hut O(N log N) approximation for large body counts
- [ ] Proper gravitational lensing ray-marching in a post-process pass
- [ ] 3D camera with tilt/rotation
- [ ] Particle effects on body collision/merge
- [ ] Save / load simulation state
- [ ] Mass histogram and energy/momentum time-series plots
- [ ] Body name labels rendered in world space
- [ ] GPU-accelerated N-body (compute shaders)

---

## Credits & Inspiration

- **kavan010/black_hole** — C++/OpenGL black hole geodesics and lensing visualisation;
  inspired the shader architecture and black hole rendering approach
- **kavan010/gravity_sim** — Gravity simulation concept that seeded the idea for
  a more feature-rich interactive sandbox
- [GLFW](https://www.glfw.org/), [GLM](https://github.com/g-truc/glm),
  [Dear ImGui](https://github.com/ocornut/imgui), [GLAD](https://github.com/Dav1dde/glad)
  — excellent open-source libraries that make this possible

---

## License

MIT — see [LICENSE](LICENSE) for details.
