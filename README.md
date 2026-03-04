# Animal Experiment Simulator 3D

A real-time 3D preclinical animal experiment simulator built with C++17 and OpenGL. Designed for pharmaceutical R&D education and training, it provides an interactive Surgeon Simulator-style lab environment where users can administer drugs, monitor vital signs, and observe physiological responses in real time.

## Features

### Multi-Drug Pharmacology System
- **5 built-in drug compounds** (acetaminophen, cisplatin, morphine, generic toxicant, saline) with distinct pharmacokinetic profiles and organ-specific toxicity weights
- **Custom Drug Editor** — define new compounds with molecular weight, half-life, bioavailability per route, and toxicity profiles for 10 organ systems
- **Multi-drug interactions** — administer multiple drugs simultaneously; the system models drug-drug interaction effects (20% severity increase with 2+ concurrent drugs)
- **Per-drug elimination kinetics** — first-order elimination using drug-specific half-life and species metabolic rate
- **JSON persistence** — save and load custom drug definitions (`~/.animsim/custom_drugs.json`)

### 5 Experiment Types
| Type | Description |
|------|-------------|
| **Toxicology** | Acute/chronic toxicity studies with LD50 estimation |
| **Pharmacokinetics** | Absorption, distribution, metabolism, excretion modeling |
| **Behavioral** | Behavioral scoring paradigms (Morris water maze, etc.) |
| **Drug Efficacy** | Therapeutic efficacy vs. disease models |
| **Skin Irritation** | Draize-style dermal irritation scoring |

### 6 Animal Species
Mouse, Rat, Rabbit, Guinea Pig, Dog, and Monkey — each with species-specific physiology parameters (weight, heart rate, temperature, respiratory rate, metabolic rate).

### Interactive Mode
Surgeon Simulator-style hands-on lab bench with 16 procedure types:

- **Dosing**: Oral gavage, IV/IP/SC injection, dermal application
- **Monitoring**: Blood sample, weigh, temperature, observe, behavioral test, skin scoring
- **Intervention**: Saline flush (-30% drug conc.), activated charcoal (-50%), antidote (-90% target drug)
- **Terminal**: Euthanize, necropsy

### Real-Time Monitoring
- **Vital sign sparklines** — heart rate, temperature, SpO2 plotted over time via ImPlot
- **Drug concentration-time curves** — per-drug plasma concentration with procedure event markers
- **Per-drug breakdown** — individual drug concentrations, cumulative doses, and route-specific bioavailability
- **10-organ health tracking** — liver, kidney, heart, lungs, brain, stomach, intestines, skin, spleen, bone marrow
- **Blood chemistry panel** — WBC, RBC, platelets, hemoglobin, ALT, AST, BUN, creatinine, glucose, albumin
- **Delta display** — before/after vital sign comparison after each procedure

### 3D Visualization
- **Procedural animal models** — species-accurate mesh generation (no external model files needed)
- **Breathing animation** — chest oscillation driven by respiratory rate
- **Death animation** — smooth 90-degree roll with ease-out interpolation
- **Cyanosis** — progressive blue tinting when SpO2 drops below 95%
- **Health-based coloring** — animals visually show declining health
- **Procedure tool animations** — syringe, gavage tube, thermometer approach/inject/retract cycle
- **Injection particles** — color-coded particle effects (amber for drugs, red for blood, blue for oral)
- **Full lab environment** — bench, scale, beakers, test tubes, clipboard, overhead lighting, cabinets

## Tech Stack

| Component | Library |
|-----------|---------|
| Graphics | OpenGL 3.3 Core Profile, GLAD |
| Windowing | GLFW 3.x |
| GUI | Dear ImGui (docking branch) |
| Plots | ImPlot |
| Math | GLM 1.0 |
| JSON | nlohmann/json 3.11 |
| Language | C++17 |
| Build | CMake 3.20+ |

All dependencies except GLFW are fetched automatically via CMake FetchContent.

## Building

### Prerequisites (Ubuntu/WSL)

```bash
sudo apt install build-essential cmake git \
    libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libwayland-dev libxkbcommon-dev
```

### Build & Run

```bash
# Quick start (installs deps if needed, builds, and launches)
./start.sh

# Or manually:
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./AnimalSimulator
```

## Project Structure

```
src/
  core/           Application loop, window, timer
  renderer/       OpenGL renderer, camera, mesh, shader, framebuffer
  scene/          3D lab scene, procedural animal entities, procedure animations
  simulation/     Animal physiology, drug compounds, drug registry,
                  experiment base class, simulation engine, random engine
    experiments/  Toxicology, PK, Behavioral, Drug Efficacy, Skin Irritation
  interactive/    Interactive mode controller (procedures, time control)
  gui/            ImGui HUD panels (dashboard, setup, simulation, interactive,
                  results, drug editor, status bar)
  utils/          stb_image implementation
external/
  glad/           Pre-generated OpenGL loader (header-only)
assets/           Placeholder directories for shaders/models/textures
```

## Usage

1. **Dashboard** — Select an experiment type or open the Drug Lab to create custom compounds
2. **Setup** — Configure species, group count, animals per group, duration, drug, and route. Choose Automated or Interactive mode
3. **Interactive Mode** — Click procedure tools to administer drugs, take measurements, and advance time. Monitor vitals in real time with sparklines and concentration curves
4. **Automated Mode** — Watch the simulation run with auto-triggered procedure animations
5. **Results** — View experiment summary statistics

### Custom Drug Workflow

1. Open **Drug Lab** from the Dashboard
2. Click **Create New Drug**
3. Set pharmacokinetic parameters (half-life, bioavailability per route, volume of distribution)
4. Adjust toxicity sliders for each organ system (0 = no damage, 1 = primary target organ)
5. Set dose-response values (therapeutic dose, NOAEL, LD50)
6. **Save** — the drug appears in all experiment drug selectors
7. Run an experiment with your custom drug and observe organ-specific damage patterns

## Controls

| Input | Action |
|-------|--------|
| Right-click drag | Rotate camera |
| Middle-click drag | Pan camera |
| Scroll wheel | Zoom in/out |
| ESC | Quit |

## License

[Apache License 2.0](LICENSE)
