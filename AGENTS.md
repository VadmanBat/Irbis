# Agent notes (Grok Build / AI)

## Read first

1. **[docs/architecture.md](docs/architecture.md)** — layers, data flow, file map, where to change what  
2. **[README.md](README.md)** — build, deps, tab overview  
3. **[docs/ux-ui-recommendations.md](docs/ux-ui-recommendations.md)** — UI roadmap (optional unless UI work)

## Project rules (skills)

Apply user skills when editing C++/Qt:

- `cpp-my-style` — naming, private-first class layout, **split large .cpp (~100–150 lines)**
- `qt-cpp` — QObject public API camelCase, members `_`
- `high-performance-cpp` — simple structures, reserve/move where it matters

## Folder layout

Domain folders (`widgets/`, `charts/`, `dialogs/`, `tabs/`, `control/`).  
**Multi-file modules** live in subfolders (not deeper nesting):

| Module | Path |
|--------|------|
| `TranFuncForm` | `code/widgets/tf-form/` |
| `chart_utils` | `code/charts/utils/` |
| `ChartViewerWindow` | `code/dialogs/chart-viewer/` |
| `regulator_design` | `code/control/` |

Single-file classes stay flat in the domain folder.

## Math vs UI

- **numina** = transfer functions, responses, identification, regulator design  
- **RegValve** = Qt UI + thin adapters (`tf_builder`, `regulator_design`, charts)

Do not reimplement poly/TF math in the app.

## CMake

New sources → **`cmake/source-files.cmake`** (paths under module folders).

## Build tip

If link fails with `Permission denied` on `RegValve.exe`, stop the running app and rebuild.
