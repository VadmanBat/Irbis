# Irbis

Desktop-приложение (Qt 6) для анализа передаточных функций и ручной настройки регулятора.  
Математика — внешняя библиотека **[numina](https://github.com/VadmanBat/numina)**; UI — Qt Designer (`.ui`).

## Зависимости

- CMake ≥ 3.28, C++23
- Qt 6 (Widgets, Charts) — по умолчанию MSYS2 UCRT64: `C:/msys64/ucrt64`
- numina по пути `C:/cpp/projects/static-libs/numina` (или `-DNUMINA_ROOT=...`)

## Сборка

```bash
# toolchain MSYS2 UCRT64 в PATH (g++, ninja)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64 ^
  -DQt6_DIR=C:/msys64/ucrt64/lib/cmake/Qt6
cmake --build build
```

В CLion: kit с компилятором `C:/msys64/ucrt64/bin/g++.exe`, CMake option  
`-DCMAKE_PREFIX_PATH=C:/msys64/ucrt64` (или уже подхватится из `CMakeLists.txt`).

**PATH:** для `moc`/`uic` CMake ставит обёртки (`cmake/msys-qt-env.cmake`), чтобы не было  
`0xC0000135` (нет DLL). Дополнительно можно в CLion → CMake profile → Environment:  
`PATH=C:\msys64\ucrt64\bin;%PATH%`.

Рабочий каталог для запуска: рядом с exe должны быть (или пути из `data/`):

- `data/fonts/...`
- `data/styles/button-style.qss` (или `styles/button-style.qss`)

## Структура

```
ui/                 # .ui (kebab-case)
code/app/           # MainWindow
code/tabs/          # id-tab, analysis-tab, synthesis-tab, rim-tab
code/dialogs/       # dialogs
code/widgets/       # tran-func-form, tf-display-widget, …
code/series/        # Series, ComplexSeries, SetSeries
code/charts/        # chart-utils, chart-panel, response-chart-bank
code/util/          # tf-builder, format.hxx, data-file-parser
```

Файлы: **kebab-case**. Числа: `num_format::SIGNIFICANT_DIGITS` в `format.hxx`.

## Вкладки

1. **Идентификация** (`IdTab`) — h(t) / (t,u,y) → Симою (статика) или k/p (астатика)
2. **Анализ** (`AnalysisTab`) — W(p), отклики, качество
3. **Синтез** (`SynthesisTab`) — регулятор, замкнутый контур
4. **Настройка РИМ** (`RimTab`) — W(p), `PidSettings`, уставка; y(t) реального РИМ и идеального W_reg; продолжение прогона

## Документация

- [Архитектура](docs/architecture.md) — слои, потоки данных, карта файлов (для разработки / Grok Build)
- [AGENTS.md](AGENTS.md) — краткие правила для AI-агентов
- [UX/UI-рекомендации](docs/ux-ui-recommendations.md) — компоновка экранов, форма ПФ, roadmap
- [Эскизы UI](docs/sketches/index.html) — wireframes вариантов (открыть в браузере)


## License

См. `LICENSE`.

