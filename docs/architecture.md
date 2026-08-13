# Architecture — RegValve (for Grok Build)

This document is the **map of the codebase** for future AI/human work: layers, ownership,
where to change what, and coding conventions (skills).

Related: [UX/UI recommendations](ux-ui-recommendations.md), [UI sketches](sketches/index.html).

---

## 1. Purpose

**RegValve** is a Qt 6 desktop app for control-engineering workflows:

1. **Identification** — experimental data → plant TF (Simoyu / Duhamel + optional τ)
2. **Analysis** — plant TF → time/frequency responses + quality metrics
3. **Synthesis** — plant + PID-family regulator → closed loop + metrics
4. **RIM / RKCH** — planned (placeholder tab / modes)

**Math** lives in external static library **[numina](https://github.com/VadmanBat/numina)**.  
**RegValve** is the UI + thin adapters (builders, factories, chart presentation).

---

## 2. High-level layers

```
┌─────────────────────────────────────────────────────────┐
│  main.cpp  →  MainWindow (tabs shell, fonts, QSS)       │
├─────────────────────────────────────────────────────────┤
│  Tabs (QWidget + .ui)                                   │
│    IdTab | AnalysisTab | SynthesisTab | RimTab          │
├─────────────────────────────────────────────────────────┤
│  Widgets / dialogs                                       │
│    TranFuncForm, TfDisplayWidget, RegParameter, …       │
│    ModParDialog, ChartViewerWindow, ChartDialog, …      │
├─────────────────────────────────────────────────────────┤
│  Charts                                                  │
│    ResponseChartBank → ChartPanel → chart_utils         │
│    InteractiveChartView (viewer zoom/pan)               │
├─────────────────────────────────────────────────────────┤
│  Adapters (header-mostly)                                │
│    tf_builder | data_file_parser | num_format | nice_axis│
│    BoundsSet                                              │
├─────────────────────────────────────────────────────────┤
│  numina (TransferFunction, ResponseLab, Simoyu, …)      │
└─────────────────────────────────────────────────────────┘
```

**Rule:** do not put heavy math in tabs/widgets. Call `tf_builder` / numina; keep UI reactive.

---

## 3. Directory map

| Path | Role |
|------|------|
| `main.cpp` | `QApplication`, locale, `MainWindow` |
| `code/app/` | Main window shell only |
| `code/tabs/` | One feature screen per tab (+ `*-run.cpp` for heavy logic) |
| `code/widgets/` | Single-file controls (`double-slider`, `reg-parameter`, …) |
| `code/widgets/tf-form/` | **Module:** `TranFuncForm` (multi-cpp) |
| `code/dialogs/` | Modal dialogs (one class ≈ one pair of files) |
| `code/dialogs/chart-viewer/` | **Module:** detached chart viewer window |
| `code/charts/` | `ChartPanel`, `ResponseChartBank`, `InteractiveChartView` |
| `code/charts/utils/` | **Module:** `chart_utils`, nice axes, clone |
| `code/series/` | Axis bounds aggregation (`AxisBounds`, `BoundsSet`) |
| `code/model/` | POD settings (`ModelParam`, `IdSettings`) |
| `code/control/` | (reserved; regulator coeffs → numina `makeRegulator`) |
| `code/util/` | Parsing, formatting, TF builders |
| `ui/` | Qt Designer forms (kebab-case), parallel to `code/` |
| `data/` | QSS, fonts (copied next to exe on build) |
| `docs/` | Architecture, UX, sketches |
| `cmake/` | Source list, flags, MSYS Qt env |

### 3.1 Module folders (multi-file classes)

Rule: **one class / 1–2 files → flat domain folder**; **one class / 3+ sources → own subfolder**.

```
code/widgets/tf-form/          TranFuncForm (+ edit, io, name, line-edit)
code/charts/utils/             chart_utils (+ axes, series, menu), nice-axis
code/dialogs/chart-viewer/     ChartViewerWindow (+ ui)
```

Includes use the full path from project root, e.g.:

```cpp
#include "code/widgets/tf-form/tran-func-form.h"
#include "code/charts/utils/chart-utils.hpp"
#include "code/dialogs/chart-viewer/chart-viewer-window.h"
```

---

## 4. Key types and data flow

### 4.1 Model parameters

`ModelParam` (`code/model/model-param.hpp`) — shared simulation settings:

- time: `autoTimeRange`, `timeMin`/`timeMax`, `autoTimeIntervals`, `timeIntervals`
- frequency: same pattern; **always log ω-grid**
- `approxOrder` — Padé order for delay

Edited by `ModParDialog`. Each tab owns a `ModelParam` instance (not yet a shared session).

### 4.2 Plant TF pipeline

```
UI coefficients (TranFuncForm)
    → tf_builder::plant(num, den, tau, order)
    → numina::TransferFunction
    → ResponseChartBank::appendFromTf / replaceLastFromTf
         → tf_builder::transient / impulse / frequencyBundle
         → ChartPanel series + BoundsSet + niceAxisRange
```

Closed loop (synthesis):

```
plant + TransferFunction::makeRegulator(Kp,Tu,Td)
    → tf_builder::closedLoop(...) / TransferFunction::closed
```

### 4.3 Identification

```
file → data_file_parser → step or (valve, signal)
    → DuhamelSolver? → h(t)
    → SimoyuIdentifier → plant + optional tau
    → TfDisplayWidget + charts overlay (experiment vs model)
```

Logic lives in `id-tab-run.cpp` (keep UI wiring in `id-tab.cpp`).

### 4.4 Charts

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

Snap picks 1–2–5 step with **minimal** expansion (avoids old 160→200).  
Guides `hor-line` / `ver-line` are not cloned into the viewer legend.

Opening viewer: context menu **«Открыть в окне…»** or **double-click** on a panel chart.

---

## 5. File splitting convention (skills)

Per **cpp-my-style**: class implementations split into **~100–150 line** `.cpp` units by concern.

| Class / area | Files (under module path) |
|--------------|---------------------------|
| `TranFuncForm` | `widgets/tf-form/tran-func-form.cpp` (+ `-edit`, `-io`, `-name`, `*-line-edit.hpp`) |
| `ChartViewerWindow` | `dialogs/chart-viewer/chart-viewer-window.cpp` (+ `-ui.cpp`) |
| `InteractiveChartView` | `charts/interactive-chart-view.h/.cpp` |
| `chart_utils` | `charts/utils/chart-utils.cpp` (+ `-axes`, `-series`, `-menu`, `*-detail.hpp`, `nice-axis`) |
| `ResponseChartBank` | `charts/response-chart-bank.cpp` (+ `-data.cpp`) |
| `C0C1Chart` | `charts/c0-c1-chart.h/.cpp` (РКЧХ locus + click) |
| `IdTab` / `SynthesisTab` | `tabs/*-tab.cpp` (+ `*-run.cpp`) |

When adding a large method: **new cpp unit**, not grow past ~150 lines.

CMake list: `cmake/source-files.cmake` — **register every new `.cpp`**.

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
| New regulator type | numina `TransferFunction::makeRegulator`, synthesis UI |
| New chart type | `ResponseChartBank`, `ChartVisibility`, menu labels |
| Axis styling / nice limits | `nice-axis.hpp`, `chart-utils` guides |
| TF clipboard format | `widgets/tf-form/*` IO (`RegValve-TF-v1`) |
| Identification algorithm | prefer **numina**; UI only in `id-tab-run` |
| Auto-synthesis PI/PID (РКЧХ / Γ) | `SynthesisTab::autoSynthesize` + numina `RegulatorDesigner`; UI: `C0C1Chart`, φ, criterion, law, region |
| Global chrome / buttons | `data/styles/app.qss` |
| Window shell / tabs list | `mainwindow.cpp` |
| Chart zoom window | `dialogs/chart-viewer/*`, `charts/interactive-chart-view.*` |

---

## 8. Build & dependencies

- **CMake ≥ 3.28**, **C++23**, **Qt6** Widgets + Charts
- **numina** via `NUMINA_ROOT` (`CMakeLists.txt`)
- MSYS2 UCRT64 helpers: `cmake/msys-qt-env.cmake`
- Runtime assets: `data/` copied next to exe (POST_BUILD)

Tests (optional): `REGVALVE_BUILD_TESTS` → `nice_axis_test` (pure math, no Qt UI).

---

## 9. Known gaps / future architecture

1. **No shared session model** — each tab holds its own `ModelParam` / TF; UX doc recommends a session `PlantModel`.
2. **RimTab / RKCH** — placeholder; keep modes under Synthesis when implementing.
3. **ПИ / ПИД / Авто** — `designPi` / `designPid` / `design()` (закон не подменяется); ПИД на `C₂*(ω)`; Γ — сектор только для ПИ. СКО настройки — H₂, не `QualityReport::sigma`.
4. **UI polish** — see `docs/ux-ui-recommendations.md` (cards, TF read/edit modes).

---

## 10. Guidance for Grok Build agents

1. Read this file + `README.md` before large changes.
2. Prefer **editing adapters** over reimplementing numina math.
3. Keep **tabs thin**: move algorithms to `*-run.cpp` / `util/` / numina.
4. Split large new code into **multiple kebab-case `.cpp`** (~100–150 lines).
5. Update `cmake/source-files.cmake` when adding sources.
6. After chart/TF behavior changes, rebuild Release kit (watch for locked `RegValve.exe`).
7. Do not expand scope into UX redesign unless asked — architecture here is structural.

---

*Last structural refactor: chart stack coherence (`SeriesWrite`, one-pass bounds, `GridMode` Tab/Viewer, split `chart-utils-axes`).*
