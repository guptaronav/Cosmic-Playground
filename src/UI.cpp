#include "UI.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cmath>
#include <cstring>

// Dark-space theme
static void applyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding   = 8.f;
    s.FrameRounding    = 5.f;
    s.GrabRounding     = 5.f;
    s.ScrollbarRounding= 5.f;
    s.WindowPadding    = {12.f, 10.f};
    s.FramePadding     = {8.f,  4.f};
    s.ItemSpacing      = {8.f,  5.f};
    s.Alpha            = 0.95f;

    auto* c = s.Colors;
    c[ImGuiCol_WindowBg]       = {0.07f, 0.06f, 0.12f, 0.92f};
    c[ImGuiCol_TitleBg]        = {0.10f, 0.08f, 0.18f, 1.f};
    c[ImGuiCol_TitleBgActive]  = {0.14f, 0.10f, 0.28f, 1.f};
    c[ImGuiCol_Header]         = {0.20f, 0.15f, 0.38f, 0.80f};
    c[ImGuiCol_HeaderHovered]  = {0.30f, 0.22f, 0.55f, 0.80f};
    c[ImGuiCol_HeaderActive]   = {0.38f, 0.28f, 0.65f, 1.f};
    c[ImGuiCol_FrameBg]        = {0.12f, 0.10f, 0.22f, 0.90f};
    c[ImGuiCol_FrameBgHovered] = {0.18f, 0.14f, 0.32f, 0.90f};
    c[ImGuiCol_SliderGrab]     = {0.55f, 0.35f, 0.90f, 1.f};
    c[ImGuiCol_SliderGrabActive]= {0.70f, 0.50f, 1.00f, 1.f};
    c[ImGuiCol_Button]         = {0.20f, 0.14f, 0.40f, 0.90f};
    c[ImGuiCol_ButtonHovered]  = {0.30f, 0.20f, 0.60f, 1.f};
    c[ImGuiCol_ButtonActive]   = {0.42f, 0.28f, 0.80f, 1.f};
    c[ImGuiCol_CheckMark]      = {0.70f, 0.50f, 1.00f, 1.f};
    c[ImGuiCol_Text]           = {0.92f, 0.90f, 1.00f, 1.f};
    c[ImGuiCol_Separator]      = {0.25f, 0.20f, 0.45f, 1.f};
    c[ImGuiCol_Border]         = {0.25f, 0.20f, 0.40f, 0.60f};
}

void UI::init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename  = nullptr;  // don't save layout

    applyTheme();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410 core");

    // Initialise the pending body defaults
    pendingBody       = Body{};
    pendingBody.name  = "New Planet";
    pendingBody.type  = BodyType::Planet;
    pendingBody.mass  = 1.0;
    pendingBody.radius= Body::defaultRadius(BodyType::Planet);
    pendingBody.color = Body::defaultColor(BodyType::Planet);
}

void UI::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UI::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UI::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ── Main draw ──────────────────────────────────────────────────────────────────
bool UI::draw(Simulation& sim, Renderer& renderer, Camera& cam,
              const Presets& presets, float fps, float simTime) {

    panelSimControls(sim, fps, simTime);
    panelAddBody(sim, cam);
    panelSelectedBody(sim);
    panelPresets(sim, presets, cam);
    panelVisuals(renderer);
    panelPhysics(sim);
    panelStats(sim, fps);

    return ImGui::GetIO().WantCaptureMouse;
}

// ── Panel: Simulation controls ─────────────────────────────────────────────────
void UI::panelSimControls(Simulation& sim, float fps, float simTime) {
    ImGui::SetNextWindowPos({10.f, 10.f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({230.f, 170.f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Simulation", nullptr, ImGuiWindowFlags_NoCollapse);

    auto& cfg = sim.config();

    // Play/Pause toggle
    if (cfg.paused) {
        if (ImGui::Button("  Play  "))  cfg.paused = false;
    } else {
        if (ImGui::Button(" Pause  "))  cfg.paused = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("  Reset ")) {
        sim.clearBodies();
    }

    ImGui::Separator();
    ImGui::Text("FPS: %.0f", fps);
    ImGui::Text("Time: %.1f s", simTime);
    ImGui::Text("Bodies: %zu", sim.bodies().size());

    ImGui::Separator();
    float ts = static_cast<float>(cfg.timeScale);
    if (ImGui::SliderFloat("Speed", &ts, 0.1f, 10.f, "%.1fx"))
        cfg.timeScale = ts;

    ImGui::Checkbox("Trails",    &cfg.trailsEnabled);
    ImGui::SameLine();
    ImGui::Checkbox("Grid",      &cfg.gridEnabled);
    ImGui::SameLine();
    ImGui::Checkbox("Absorb",    &cfg.absorptionOn);

    ImGui::End();
}

// ── Panel: Add body ────────────────────────────────────────────────────────────
void UI::panelAddBody(Simulation& sim, Camera& /*cam*/) {
    ImGui::SetNextWindowPos({10.f, 195.f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({230.f, 240.f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Add Body", nullptr, ImGuiWindowFlags_NoCollapse);

    // Name
    char nameBuf[64];
    std::strncpy(nameBuf, pendingBody.name.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf)-1] = '\0';
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
        pendingBody.name = nameBuf;

    // Type combo
    static const char* typeNames[] = {"Planet", "Star", "Neutron Star", "Black Hole"};
    int typeIdx = static_cast<int>(pendingBody.type);
    if (ImGui::Combo("Type", &typeIdx, typeNames, 4)) {
        pendingBody.type   = static_cast<BodyType>(typeIdx);
        pendingBody.color  = Body::defaultColor(pendingBody.type);
        pendingBody.radius = Body::defaultRadius(pendingBody.type);
    }

    // Mass
    float mass = static_cast<float>(pendingBody.mass);
    if (ImGui::SliderFloat("Mass", &mass, 0.05f, 5000.f, "%.1f", ImGuiSliderFlags_Logarithmic))
        pendingBody.mass = mass;

    float rad = pendingBody.radius;
    if (ImGui::SliderFloat("Radius", &rad, 0.05f, 3.f))
        pendingBody.radius = rad;

    // Colour
    float col[4] = {pendingBody.color.r, pendingBody.color.g,
                    pendingBody.color.b, pendingBody.color.a};
    if (ImGui::ColorEdit4("Color", col))
        pendingBody.color = {col[0], col[1], col[2], col[3]};

    ImGui::Separator();
    ImGui::TextWrapped("Shift + LClick: place body\nRClick + drag: set velocity");

    if (ImGui::Button("Add at Centre")) {
        Body b = pendingBody;
        b.position = {0.0, 0.0};
        b.velocity = {0.0, 0.0};
        sim.addBody(b);
    }

    ImGui::End();
}

// ── Panel: Selected body editor ────────────────────────────────────────────────
void UI::panelSelectedBody(Simulation& sim) {
    Body* b = sim.findBody(selectedBodyId);
    if (!b) return;

    ImGui::SetNextWindowPos({10.f, 450.f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({230.f, 220.f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Selected Body", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored({0.8f, 0.7f, 1.f, 1.f}, "%s", b->name.c_str());
    ImGui::Text("Type: %s", bodyTypeName(b->type));
    ImGui::Text("Pos: (%.2f, %.2f)", b->position.x, b->position.y);
    ImGui::Text("Speed: %.3f", glm::length(b->velocity));

    ImGui::Separator();

    float mass = static_cast<float>(b->mass);
    if (ImGui::SliderFloat("Mass##sel", &mass, 0.05f, 5000.f, "%.1f",
                           ImGuiSliderFlags_Logarithmic))
        b->mass = mass;

    if (ImGui::SliderFloat("Radius##sel", &b->radius, 0.05f, 3.f))
        {}

    float col[4] = {b->color.r, b->color.g, b->color.b, b->color.a};
    if (ImGui::ColorEdit4("Color##sel", col))
        b->color = {col[0], col[1], col[2], col[3]};

    ImGui::Separator();
    if (ImGui::Button("Remove")) {
        sim.removeBody(selectedBodyId);
        selectedBodyId = 0;
    }

    ImGui::End();
}

// ── Panel: Presets ─────────────────────────────────────────────────────────────
void UI::panelPresets(Simulation& sim, const Presets& presets, Camera& /*cam*/) {
    ImGui::SetNextWindowPos({10.f, 685.f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({230.f, 180.f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Presets", nullptr, ImGuiWindowFlags_NoCollapse);

    const auto& list = presets.list();
    for (size_t i = 0; i < list.size(); ++i) {
        if (ImGui::Button(list[i].name.c_str(), {210.f, 0.f})) {
            presets.apply(i, sim);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", list[i].description.c_str());
    }

    ImGui::End();
}

// ── Panel: Visuals ─────────────────────────────────────────────────────────────
void UI::panelVisuals(Renderer& renderer) {
    ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x - 245.f, 10.f},
                             ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({235.f, 120.f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Visuals", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::Checkbox("Trails (T)",    &renderer.showTrails);
    ImGui::Checkbox("Grid (G)",      &renderer.showGrid);
    ImGui::Checkbox("Glow",         &renderer.showGlow);

    ImGui::End();
}

// ── Panel: Physics settings ────────────────────────────────────────────────────
void UI::panelPhysics(Simulation& sim) {
    ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x - 245.f, 145.f},
                             ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({235.f, 120.f}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Physics", nullptr, ImGuiWindowFlags_NoCollapse);

    auto& cfg = sim.config();
    float G = static_cast<float>(cfg.G);
    if (ImGui::SliderFloat("G (gravity)", &G, 10.f, 5000.f, "%.0f",
                           ImGuiSliderFlags_Logarithmic))
        cfg.G = G;

    float eps = static_cast<float>(cfg.softeningEps);
    if (ImGui::SliderFloat("Softening ε", &eps, 0.01f, 2.f, "%.3f"))
        cfg.softeningEps = eps;

    ImGui::End();
}

// ── Panel: Stats overlay ────────────────────────────────────────────────────────
void UI::panelStats(const Simulation& sim, float fps) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({io.DisplaySize.x - 245.f, io.DisplaySize.y - 90.f},
                             ImGuiCond_Always);
    ImGui::SetNextWindowSize({235.f, 80.f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::Begin("##stats", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoNav        | ImGuiWindowFlags_NoMove);

    ImGui::Text("FPS: %.1f | Bodies: %zu", fps, sim.bodies().size());
    ImGui::Text("E: %.2e", sim.totalEnergy());

    auto com = sim.centreOfMass();
    ImGui::Text("CoM: (%.1f, %.1f)", com.x, com.y);

    ImGui::End();
}
