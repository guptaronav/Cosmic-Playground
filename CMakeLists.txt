cmake_minimum_required(VERSION 3.20)
project(CosmicPlayground VERSION 1.0 LANGUAGES CXX)

# Allow fetched dependencies whose CMakeLists still use old cmake_minimum_required
# versions (glad v0.1.x, glm 0.9/1.0.x) to configure under CMake 4.x.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ──────────────────────────────────────────────────────────────
# Dependencies via FetchContent
# ──────────────────────────────────────────────────────────────

# GLFW – windowing & input
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLM – header-only math
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

# ImGui – immediate-mode UI
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.5
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# GLAD – OpenGL loader (generates at configure time)
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v0.1.36
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(glad)
if(NOT glad_POPULATED)
    FetchContent_Populate(glad)
    # glad v0.1.x ships its own CMakeLists that builds the glad target
    add_subdirectory(${glad_SOURCE_DIR} ${glad_BINARY_DIR})
endif()

find_package(OpenGL REQUIRED)

# ──────────────────────────────────────────────────────────────
# Source files
# ──────────────────────────────────────────────────────────────

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

set(COSMIC_SOURCES
    src/main.cpp
    src/App.cpp
    src/Body.cpp
    src/Simulation.cpp
    src/Renderer.cpp
    src/Camera.cpp
    src/Input.cpp
    src/Presets.cpp
    src/UI.cpp
)

add_executable(CosmicPlayground
    ${COSMIC_SOURCES}
    ${IMGUI_SOURCES}
)

# ──────────────────────────────────────────────────────────────
# Include paths
# ──────────────────────────────────────────────────────────────

target_include_directories(CosmicPlayground PRIVATE
    src/
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${glad_SOURCE_DIR}/include
    ${glm_SOURCE_DIR}
)

# ──────────────────────────────────────────────────────────────
# Link libraries
# ──────────────────────────────────────────────────────────────

target_link_libraries(CosmicPlayground PRIVATE
    glfw
    glad
    OpenGL::GL
)

if(APPLE)
    target_compile_definitions(CosmicPlayground PRIVATE GL_SILENCE_DEPRECATION)
    target_link_libraries(CosmicPlayground PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

if(MSVC)
    target_compile_options(CosmicPlayground PRIVATE /W3)
else()
    target_compile_options(CosmicPlayground PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ──────────────────────────────────────────────────────────────
# Copy shaders next to the executable after each build
# ──────────────────────────────────────────────────────────────

add_custom_command(TARGET CosmicPlayground POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:CosmicPlayground>/shaders
    COMMENT "Copying shaders to build directory"
)
