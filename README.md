# Irbis

Qt 6 UI-библиотека и desktop-приложение для анализа передаточных функций и ручной настройки регулятора.  
Математика — внешняя библиотека **[numina](https://github.com/VadmanBat/numina)**; UI — Qt Designer (`.ui`).

Библиотека `irbis::irbis` можно подключить из другого приложения через `add_subdirectory` и вставить вкладки (`AnalysisTab`, `IdTab`, …) как обычные `QWidget`.

## Зависимости

- CMake ≥ 3.28, C++23
- Qt 6 (Widgets, Charts, Svg) — по умолчанию MSYS2 UCRT64: `C:/msys64/ucrt64`
- numina по пути `C:/cpp/projects/static-libs/numina` (или `-DNUMINA_ROOT=...`, либо уже существующий target `numina::numina`)

## Сборка (это репозиторий как приложение)

```bash
# toolchain MSYS2 UCRT64 в PATH (g++, ninja)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64 ^
  -DQt6_DIR=C:/msys64/ucrt64/lib/cmake/Qt6
cmake --build build
```

Цель приложения: **`irbis`** (`irbis.exe`) — то же окно с вкладками, что и раньше. В CLion run target: `irbis`.

Kit: компилятор `C:/msys64/ucrt64/bin/g++.exe`, CMake option  
`-DCMAKE_PREFIX_PATH=C:/msys64/ucrt64` (или уже подхватится из `CMakeLists.txt`).

**PATH:** для `moc`/`uic` CMake ставит обёртки (`cmake/msys-qt-env.cmake`), чтобы не было  
`0xC0000135` (нет DLL). Дополнительно можно в CLion → CMake profile → Environment:  
`PATH=C:\msys64\ucrt64\bin;%PATH%`.

Шрифты и QSS вшиты в библиотеку (`:/irbis/...`). Рядом с exe по-прежнему копируется `data/` — файлы перекрывают ресурс.

## Подключение в другом приложении

```cmake
# соседний каталог / submodule / monorepo
add_subdirectory(path/to/Irbis)

add_executable(app2 main.cpp)
target_link_libraries(app2 PRIVATE irbis::irbis)
```

Если `numina::numina` уже есть в родительском проекте, Irbis его не дублирует. Иначе нужен `NUMINA_ROOT`.

Родительское приложение само настраивает Qt (`find_package`, PATH для moc на MSYS2).

```cpp
#include "irbis/irbis.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    irbis::applyStyleSheet();          // шрифты + QSS Irbis; можно не вызывать

    auto* tab = new AnalysisTab;       // или IdTab / SynthesisTab / RimTab
    tab->show();
    return QApplication::exec();
}
```

Отдельные заголовки: `"irbis/tabs/analysis-tab.h"`, `"irbis/widgets/tf-form/tran-func-form.h"`, …

## Структура

```
include/irbis/      # публичные заголовки библиотеки
src/irbis/          # реализации (.cpp)
app/                # оболочка desktop-приложения (main + MainWindow)
ui/                 # .ui (kebab-case), параллельно tabs/dialogs
data/               # QSS, шрифты, иконка exe
tests/
cmake/              # sources, top-level app/tests, MSYS Qt env
```

Файлы: **kebab-case**. Числа: `num_format::SIGNIFICANT_DIGITS` в `format.hxx`.

## Вкладки (виджеты библиотеки)

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
