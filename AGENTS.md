# Agent notes (Grok Build / AI)

## Read first

1. **[docs/architecture.md](docs/architecture.md)** — layers, data flow, file map, where to change what  
2. **[README.md](README.md)** — build, deps, `add_subdirectory`, tab overview  
3. **[docs/ux-ui-recommendations.md](docs/ux-ui-recommendations.md)** — UI roadmap (optional unless UI work)

## Project rules (skills)

Apply user skills when editing C++/Qt:

- `cpp-my-style` — naming, private-first class layout, **split large .cpp (~100–150 lines)**
- `qt-cpp` — QObject public API camelCase, members `_`
- `high-performance-cpp` — simple structures, reserve/move where it matters

## Folder layout

Library (consumed via `add_subdirectory` as `irbis::irbis`):

| Path | Role |
|------|------|
| `include/irbis/` | Public headers (`#include "irbis/tabs/analysis-tab.h"`) |
| `src/irbis/` | Implementations |
| `ui/` | Qt Designer forms for library widgets |

**Multi-file modules** live in subfolders (not deeper nesting):

| Module | Header / source |
|--------|-----------------|
| `TranFuncForm` | `include/irbis/widgets/tf-form/` + `src/irbis/widgets/tf-form/` |
| `chart_utils` | `include/irbis/charts/utils/` + `src/irbis/charts/utils/` |
| `ChartViewerWindow` | `include/irbis/dialogs/chart-viewer/` + `src/irbis/dialogs/chart-viewer/` |
| `controller_design` | `include/irbis/control/` + `src/irbis/control/` |

Desktop shell (top-level build only): `app/main.cpp`, `app/mainwindow.*`, `ui/mainwindow.ui`.

Single-file classes stay flat in the domain folder under `include/irbis/` and `src/irbis/`.

## Math vs UI

- **numina** = transfer functions, responses, identification, controller design  
- **Irbis** = Qt UI + thin adapters (`tf_builder`, `controller_design`, charts)

Do not reimplement poly/TF math in the app.

## CMake

- Library sources → **`cmake/sources.cmake`**
- App + tests (only if `PROJECT_IS_TOP_LEVEL`) → **`cmake/top-level.cmake`**
- New `.cpp` / `.ui` for the library → `cmake/sources.cmake`

## Build tip

If link fails with `Permission denied` on `Irbis.exe`, stop the running app and rebuild.  
CLion run target: **`irbis`** (same desktop app with tabs as before).
