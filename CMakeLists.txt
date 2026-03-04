cmake_minimum_required(VERSION 3.20)
project(AnimalSimulator VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Options ────────────────────────────────────────────────────────────────
option(ANIMSIM_USE_SYSTEM_GLFW "Use system-installed GLFW instead of fetching" ON)

# ─── FetchContent for dependencies ──────────────────────────────────────────
include(FetchContent)

# glm (math library) ---------------------------------------------------------
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)

# imgui (docking branch) -----------------------------------------------------
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
)

# implot (plotting for imgui) -------------------------------------------------
FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# stb (stb_image header) -----------------------------------------------------
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# tinyobjloader --------------------------------------------------------------
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        release
    GIT_SHALLOW    TRUE
)

# nlohmann/json (for drug compound persistence) ------------------------------
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

# ─── Fetch all ──────────────────────────────────────────────────────────────
message(STATUS "Fetching dependencies...")
FetchContent_MakeAvailable(glm stb tinyobjloader json)

# imgui + implot need special handling (no CMakeLists.txt)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

FetchContent_GetProperties(implot)
if(NOT implot_POPULATED)
    FetchContent_Populate(implot)
endif()

# ─── GLFW (system or fetched) ───────────────────────────────────────────────
if(ANIMSIM_USE_SYSTEM_GLFW)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW3 REQUIRED glfw3)
else()
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE
    )
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

# ─── glad (pre-generated, header-only) ──────────────────────────────────────
# Generated via: python3 -m glad --api gl:core=3.3 --out-path external/glad c --header-only
# The header defines GLAD_GL_IMPLEMENTATION in exactly one .cpp (see stb-style usage)
add_library(glad INTERFACE)
target_include_directories(glad INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include")

# ─── Build Dear ImGui as a static library ───────────────────────────────────
set(IMGUI_DIR "${imgui_SOURCE_DIR}")
set(IMPLOT_DIR "${implot_SOURCE_DIR}")

add_library(imgui_lib STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
    "${IMPLOT_DIR}/implot.cpp"
    "${IMPLOT_DIR}/implot_items.cpp"
)
target_include_directories(imgui_lib PUBLIC
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
    "${IMPLOT_DIR}"
)
target_link_libraries(imgui_lib PUBLIC glad)

if(ANIMSIM_USE_SYSTEM_GLFW)
    target_include_directories(imgui_lib PUBLIC ${GLFW3_INCLUDE_DIRS})
    target_link_libraries(imgui_lib PUBLIC ${GLFW3_LIBRARIES})
else()
    target_link_libraries(imgui_lib PUBLIC glfw)
endif()

# ─── stb_image implementation library ───────────────────────────────────────
add_library(stb_impl STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/stb_impl.cpp")
target_include_directories(stb_impl PUBLIC "${stb_SOURCE_DIR}")

# ─── tinyobjloader ──────────────────────────────────────────────────────────
set(TINYOBJ_DIR "${tinyobjloader_SOURCE_DIR}")

# ─── Main executable ────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.h"
)
# Exclude stb_impl.cpp since it's already in stb_impl library
list(FILTER SOURCES EXCLUDE REGEX "stb_impl\\.cpp$")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${TINYOBJ_DIR}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui_lib
    glad
    glm::glm
    stb_impl
    nlohmann_json::nlohmann_json
    dl
)

if(ANIMSIM_USE_SYSTEM_GLFW)
    target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW3_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GLFW3_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
endif()

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

# ─── Copy assets to build directory ─────────────────────────────────────────
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

# ─── Compiler warnings ─────────────────────────────────────────────────────
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wpedantic -Wno-unused-parameter
)
