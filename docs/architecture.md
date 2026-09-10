# Architecture — Irbis (for Grok Build)

This document is the **map of the codebase** for future AI/human work: layers, ownership,
where to change what, and coding conventions (skills).

Related: [UX/UI recommendations](ux-ui-recommendations.md), [UI sketches](sketches/index.html).

---

## 1. Purpose

**Irbis** is a Qt 6 desktop app for control-engineering workflows:

1. **Identification** — experimental data → plant TF (static: Simoyu / Duhamel + optional τ; astatic: integrator k/p)
2. **Analysis** — plant TF → time/frequency responses + quality metrics
3. **Synthesis** — plant + PID-family regulator → closed loop + metrics
4. **RIM** — discrete relay-pulse controller vs ideal W_reg(p) on the same plant

**Math** lives in external static library **[numina](https://github.com/VadmanBat/numina)**.  
**Irbis** is a Qt UI library (`irbis::irbis` / target `irbis-ui`) plus the same desktop app as before (executable `irbis`, four tabs).  
Another application can `add_subdirectory` this repo and instantiate the same tabs.

---

## 2. High-level layers

```
┌─────────────────────────────────────────────────────────┐
│  app/  main.cpp → MainWindow (optional desktop shell)   │
├─────────────────────────────────────────────────────────┤
│  irbis::irbis  (static Qt library)                      │
│    Tabs: IdTab | AnalysisTab | SynthesisTab | RimTab    │
│    Widgets / dialogs / charts / adapters                │
├─────────────────────────────────────────────────────────┤
│  numina (TransferFunction, ResponseLab, PidController, …)│
└─────────────────────────────────────────────────────────┘
```

**Rule:** do not put heavy math in tabs/widgets. Call `tf_builder` / numina; keep UI reactive.

---

## 3. Directory map

Layout matches **numina**: public headers under `include/irbis/`, implementations under `src/irbis/`.

| Path | Role |
|------|------|
| `include/irbis/` | Public API (`#include "irbis/..."`) |
| `src/irbis/` | Library `.cpp` (same relative tree as headers) |
| `app/` | Desktop shell: `main.cpp`, `MainWindow` |
| `include/irbis/tabs/` | One feature screen per tab (+ `src/.../*-run.cpp`) |
| `include/irbis/widgets/` | Single-file controls (`double-slider`, `reg-parameter`, …) |
| `include/irbis/widgets/tf-form/` | **Module:** `TranFuncForm` |
| `include/irbis/dialogs/` | Modal dialogs |
| `include/irbis/dialogs/chart-viewer/` | **Module:** detached chart viewer |
| `include/irbis/charts/` | `ChartPanel`, `ResponseChartBank`, `InteractiveChartView` |
| `include/irbis/charts/utils/` | **Module:** `chart_utils`, nice axes |
| `include/irbis/series/` | `AxisBounds`, `BoundsSet` |
| `include/irbis/model/` | POD settings (`ModelParam`, `IdSettings`) |
| `include/irbis/control/` | `controller_design`, `rim` |
| `include/irbis/util/` | Parsing, formatting, TF builders, `TfStepper` |
| `include/irbis/style.hpp` | Fonts + QSS (`irbis::loadFonts` / `applyStyleSheet`) |
| `include/irbis/irbis.h` | Umbrella: four tabs + style |
| `ui/` | Qt Designer forms (kebab-case) |
| `data/` | QSS, fonts, icons; `irbis_resources.qrc` baked into the library |
| `docs/` | Architecture, UX, sketches |
| `cmake/` | `sources.cmake` (library), `top-level.cmake` (app + tests) |

### 3.1 Module folders (multi-file classes)

Rule: **one class / 1–2 files → flat domain folder**; **one class / 3+ sources → own subfolder**.

```
include/irbis/widgets/tf-form/     TranFuncForm
src/irbis/widgets/tf-form/
include/irbis/charts/utils/        chart_utils, nice-axis
src/irbis/charts/utils/
include/irbis/dialogs/chart-viewer/
src/irbis/dialogs/chart-viewer/
```

Includes are rooted at `include/`:

```cpp
#include "irbis/widgets/tf-form/tran-func-form.h"
#include "irbis/charts/utils/chart-utils.hpp"
#include "irbis/dialogs/chart-viewer/chart-viewer-window.h"
```

### 3.2 Using Irbis from another CMake project

```cmake
add_subdirectory(path/to/Irbis)          # or FetchContent
target_link_libraries(app2 PRIVATE irbis::irbis)
```

```cpp
#include "irbis/tabs/analysis-tab.h"
auto* tab = new AnalysisTab(parent);
```

Fonts load from tab constructors. Call `irbis::applyStyleSheet()` if the host wants the Irbis QSS. Do not set `CMAKE_SOURCE_DIR` paths inside Irbis — the library uses `CMAKE_CURRENT_SOURCE_DIR`.

---

## 4. Key types and data flow

### 4.1 Model parameters

`ModelParam` (`include/irbis/model/model-param.hpp`) — shared simulation settings:

- time: `autoTimeRange`, `timeMin`/`timeMax`, `autoTimeIntervals`, `timeIntervals` → `ResponseLab` auto / range / range+N
- frequency: same three overloads (`frequency`, `amplitudeFrequency`, `phaseFrequency`). Auto + astatic \(D(0)=0\): range from pole cutoffs, lower bound several decades below (avoid \(W(j0)\))
- `approxOrder` — Padé order for delay
- `usePadeApprox` / `approxOrder` — per series at add time; `recomputeAll` updates only the time/freq grid
- exact delay: `tf_builder` binds `DelayedPlant` to `ResponseLab` (lab shifts \(h(t)\), \(W(j\omega)e^{-j\omega\tau}\), quality \(t_s\)/\(t_p\)/IAE/ISE). Irbis only converts ФЧХ rad→°

Edited by `ModParDialog`. Analysis opens the dialog with `allowIdealDelay` (checkbox + gated order). Synthesis / ID always use Padé. Each tab owns a `ModelParam` instance (not yet a shared session).

### 4.2 Plant TF pipeline

```
UI coefficients (TranFuncForm)
    → W₀ = tf_builder::plant(num, den)          [analysis]
    → or plant(num, den, τ, order)              [Padé baked in: ID / synth plant]
    → ResponseChartBank::appendFromTf(..., τ)
         → ResponseLab(DelayedPlant(W₀, τ))             if !usePadeApprox
         → ResponseLab(W₀·Padé(τ, order))               if  usePadeApprox
         → ChartPanel series + BoundsSet + niceAxisRange
```

Closed loop (synthesis):

```
plant + TransferFunction::makeController(Kp,Ti,Td)
    → tf_builder::closedLoop(...) / TransferFunction::closed
```

### 4.3 Identification

```
file → data_file_parser → step or (valve, signal)
    static:  DuhamelSolver? → h(t) → DeadTimeEstimator? → SimoyuIdentifier (Mode::Refined) → W₀
    astatic: IntegratorIdentifier (h or (u,y); no Duhamel) → k/p
    → TfDisplayWidget + one ChartPanel (experiment vs model at file times)
```

Layout: left sidebar (method, file, plant kind, ID settings, run) / right: formula + h(t).  
No quality metrics, chart-type menu, or model-parameter grid on this tab.

Logic lives in `id-tab-run.cpp` (keep UI wiring in `id-tab.cpp`).

### 4.4 RIM (relay-pulse controller)

Left sidebar: `PidSettings` + law + setpoint + horizon `T` + step `dt`.  
Top: plant `TranFuncForm`. Charts: `y(t)` (setpoint + ЗН tube / real / ideal) and `μ(t)` (real / ideal).

```
W_ОУ + τ
    → TfStepper (backward Euler via Polynomial::compose + delay line)
real:  e → numina::PidController (filter, ЗН, ПДД2, PWM, ИМ)
ideal: e → TfStepper of TransferFunction::makeController (эталонный ПИД, без Td/8)
```

**Моделировать** starts a new `Session` (resets state and charts).  
**Продолжить** keeps controller/plant state and old points; only setpoint and extra time change. `dt` and RIM settings stay those of the first run.

Do not reimplement PWM / PDD2 — call `numina::PidController`. `TfStepper` is the thin discrete plant (numina prints the Euler equations, Irbis runs them).

### 4.5 Charts

Ownership (top → bottom):

| Layer | Role |
|-------|------|
| **`ResponseChartBank`** | 5 panels, visibility, `history_` batches, `BoundsSet` per panel, lazy materialize |
| **`ChartPanel`** | owns `QChart` + `QChartView`; add/replace/fit/clear |
| **`chart_utils`** | free functions on bare `QChart*` — no UI state |
| **`InteractiveChartView`** | pan/zoom + `GridMode::Viewer` (detached window) |
| **`ChartViewerWindow`** | non-modal clone via `cloneChart` |

**Series write path (one pass):**  
`toPointsWithBounds` → `QLineSeries` + `AxisBounds` → `SeriesWrite{wrote, bounds}`.  
Visible panels: bounds from write. Hidden: `boundsOf*` only; series built on show from `history_`.

**Axis policy (`ChartPanel::fitAxes`):**

| Axis | Flag | Range + grid |
|------|------|----------------|
| t / ω | `niceX=false` | `dataAxisRange`, Fixed exact (no snap → ω stays off 0) |
| value Y | `niceY=true` | `niceAxisRange` + snap lattice through 0 |
| КЧХ X/Y | both nice | same: majors at …,−s,0,s,… so grid crosses origin |
| C₀–C₁ / C₁–C₂ | snap X/Y | 1–2–5 from 0; labels `%.4g`; ПИ: C₁–C₀; ПД: C₁–C₂; П/И без области; `+` pins selection with series color |

Snap picks 1–2–5 step with **minimal** expansion (avoids old 160→200).  
Guides `hor-line` / `ver-line` are not cloned into the viewer legend.

Opening viewer: context menu **«Открыть в окне…»** or **double-click** on a panel chart.

---

## 5. File splitting convention (skills)

Per **cpp-my-style**: class implementations split into **~100–150 line** `.cpp` units by concern.

| Class / area | Files (under module path) |
|--------------|---------------------------|
| `TranFuncForm` | `src/irbis/widgets/tf-form/tran-func-form.cpp` (+ `-edit`, `-io`, `-name`) |
| `TranFuncDialog` | `src/irbis/dialogs/tran-func-dialog.cpp` (+ `-poles.cpp`) |
| `ChartViewerWindow` | `src/irbis/dialogs/chart-viewer/chart-viewer-window.cpp` (+ `-ui.cpp`) |
| `InteractiveChartView` | `charts/interactive-chart-view.h` + `src/irbis/charts/...` |
| `chart_utils` | `src/irbis/charts/utils/chart-utils.cpp` (+ `-axes`, `-series`, `-menu`, `*-detail.hpp`) |
| `ResponseChartBank` | `src/irbis/charts/response-chart-bank.cpp` (+ `-data.cpp`) |
| `C0C1Chart` | `c0-c1-chart.h` + `.cpp` / `-axes` / `-pointer` |
| `controller_design` | `include/irbis/control/controller-design.hpp` + `src/...` |
| `IdTab` / `SynthesisTab` / `RimTab` | `src/irbis/tabs/*-tab.cpp` (+ `*-run.cpp`, id `*-identify.cpp`, synthesis `*-synth.cpp` / `*-apply.cpp` / `*-face.cpp`) |

When adding a large method: **new cpp unit**, not grow past ~150 lines.

CMake list: `cmake/sources.cmake` — **register every new library `.cpp` / `.ui`**. App files go in `cmake/top-level.cmake`.

---

## 6. Coding conventions (summary)

Full rules: `~/.grok/skills/cpp-my-style`, `qt-cpp`, `high-performance-cpp`.

| Item | Rule |
|------|------|
| Files | kebab-case |
| Classes | PascalCase |
| Public methods | camelCase (Qt-style on `QObject`) |
| Private methods | snake_case |
| Members | trailing `_` |
| Locals | snake_case |
| Class layout | private data → private methods → public API |
| Ownership | QObject parent tree; non-owning raw ptr/ref; else smart ptr |
| Validation | caller validates; keep hot paths lean |
| Headers | hot/small/templates in `.hpp`; constants `.hxx` |

---

## 7. Where to change what (cheat sheet)

| Task | Touch |
|------|--------|
| New simulation parameter | `ModelParam`, `mod-par-dialog.ui` + `.cpp`, `tf_builder` |
| New controller type | numina `TransferFunction::makeController`, synthesis UI |
| New chart type | `ResponseChartBank`, `ChartVisibility`, menu labels |
| Axis styling / nice limits | `nice-axis.hpp`, `chart-utils` guides |
| TF clipboard format | `widgets/tf-form/*` IO (`Irbis-TF-v1`, reads legacy `RegValve-TF-v1`) |
| Identification algorithm | prefer **numina** (`SimoyuIdentifier` / `IntegratorIdentifier`); UI in `id-tab` + `id-tab-run` |
| RIM closed-loop sim | `RimTab` + `TfStepper` + `rim::idealPair`; math: `numina::PidController` |
| Auto-synthesis P/I/PD/PI/PID (РКЧХ / Γ) | `controller_design` + `SynthesisTab::autoSynthesize`; UI: `C0C1Chart` (ПИ: C₁–C₀, ПД: C₁–C₂; П/И без области) / `Wр` face, φ, criterion, law, region |
| Slider range / intervals | `SliderSettingsDialog` from `RegParameter` ⚙ |
| TF inspector (poles, h(t), w(t), DE) | `dialogs/tran-func-dialog*`, `widgets/formula-view` |
| Global chrome / buttons | `data/styles/app.qss` |
| Window shell / tabs list | `ui/mainwindow.ui` + `app/mainwindow.cpp` (styles, fonts) |
| Chart zoom window | `dialogs/chart-viewer/*`, `charts/interactive-chart-view.*` |

---

## 8. Build & dependencies

- **CMake ≥ 3.28**, **C++23**, **Qt6** Widgets + Charts + Svg
- Library target: `irbis` / alias **`irbis::irbis`**
- Top-level executable: **`irbis`** (`irbis.exe`, CLion run target)
- **numina** via `NUMINA_ROOT` unless `numina::numina` already exists
- MSYS2 UCRT64 helpers: `cmake/msys-qt-env.cmake` (top-level only)
- QSS/fonts: qrc in the library (`:/irbis/...`); `data/` still copied next to the desktop exe

Tests (top-level, `IRBIS_BUILD_TESTS`): `nice_axis_test`, `tf_builder_test`, `tf_stepper_test`, `data_file_parser_test`, `controller_design_test`.

---

## 9. Known gaps / future architecture

1. **No shared session model** — each tab holds its own `ModelParam` / TF; UX doc recommends a session `PlantModel`.
2. **RKCH** lives under Synthesis (`C0C1Chart`); RimTab is the discrete РИМ loop.
3. **П / И / ПД / ПИ / ПИД / Авто** — `designP` / `designI` / `designPd` / `designPi` / `designPid` / `design()` (закон не подменяется). ПД: ЛРЗ в `(C₁,C₂)`; П и И — точка, без области. ПИД на `C₂*(ω)`; Γ — сектор только для ПИ. СКО настройки — H₂, не `QualityReport::sigma`.
4. **UI polish** — see `docs/ux-ui-recommendations.md` (cards, TF read/edit modes).

---

## 10. Guidance for Grok Build agents

1. Read this file + `README.md` before large changes.
2. Prefer **editing adapters** over reimplementing numina math.
3. Keep **tabs thin**: move algorithms to `*-run.cpp` / `util/` / numina.
4. Split large new code into **multiple kebab-case `.cpp`** (~100–150 lines).
5. Update `cmake/sources.cmake` when adding library sources.
6. After chart/TF behavior changes, rebuild Release kit (watch for locked `Irbis.exe`).
7. Do not expand scope into UX redesign unless asked — architecture here is structural.

---

*Last structural refactor: library layout `include/irbis` + `src/irbis`, CMake `irbis::irbis` for `add_subdirectory`.*
