# AuraLite Horror Game / Anxiety Fog

Процедурный first-person horror prototype на **C++17 + SDL2 + OpenGL 3.3 / OpenGL ES 3.0**.

В проекте нет внешних ассетов: лес, дом, колодец, машина, предметы, туман, UI-шрифт, глаза и силуэты генерируются кодом.

## Что внутри

- 6-дневный horror loop с нарастающей угрозой.
- Процедурный рельеф, деревья, домик, колодец, машина и предметы.
- FPS-камера: WASD, мышь, Shift для бега.
- Мобильное управление: виртуальный джойстик, touch-look и ACTION-кнопка.
- Динамический туман, фонарь, красные глаза, силуэты и преследователь.
- HUD: текущий день, цель, координаты, расстояние до цели, stamina, panic и battery bars.
- Новые игровые системы: stamina, panic/fear, flashlight battery, LIGHT toggle, adaptive chaser pressure.
- Локализация через внешние `.lang` файлы: English, Russkiy, Espanol. Переключение — `F2`.

## Управление

| Действие | Desktop | Android |
|---|---|---|
| Ходьба | WASD | левый виртуальный джойстик |
| Обзор | мышь | правая область экрана |
| Бег | Shift | сильное отклонение джойстика |
| Действие | E / Enter | ACTION |
| Фонарь | F | LIGHT |
| Сменить язык | F2 | — |
| Освободить мышь | Escape | Back / Escape |

## Цели по дням

1. Исследовать лес и лечь спать в домике.
2. Проверить колодец `X:-20 Z:-50`, затем вернуться в домик.
3. Собрать 3 бревна вокруг домика.
4. Найти страницу дневника `X:-35 Z:-40`, затем вернуться.
5. Найти крест `X:45 Z:-45`, затем вернуться.
6. Добежать до машины `X:10 Z:75` и сбежать.

## Локализация через `.lang`

Языки лежат в папке `lang/`:

- `lang/en.lang`
- `lang/ru.lang`
- `lang/es.lang`

Формат простой:

```ini
key = value
some.text = Line one\nLine two
```

Переключение языка в игре: `F2`.

Также можно задать язык перед запуском:

```bash
AURALITE_LANG=ru ./AnxietyHorror
```

Русский перевод теперь настоящий UTF-8 на кириллице. UI-рендерер дополнен компактным пиксельным кириллическим шрифтом; латиница продолжает использовать старый 8x8 ASCII atlas.

## Сборка на Windows

Самый простой способ:

```bat
build_windows.bat
```

Или вручную:

```bat
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

После сборки исполняемый файл будет называться `AnxietyHorror.exe`.

## Сборка на Linux/macOS

Требования:

- CMake 3.16+
- C++17 compiler
- OpenGL development libraries
- SDL2 development package, либо доступ к интернету для автоматического FetchContent SDL2

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/AnxietyHorror
```

## Android

Android-часть оставлена как заготовка под SDLActivity + CMake/NDK. Для полноценной APK-сборки нужно подключить SDL2 Android Java/native слой: AAR или исходники SDL2 в Gradle-проект. C++-код уже содержит ветку `__ANDROID__` и использует GLES3.

## Недавние улучшения

- Исправлена ACTION-кнопка на Android: событие больше не сбрасывается до обработки целей.
- Убрана критическая утечка: предметы больше не пересоздают `billboardQuad` и GPU-buffer каждый кадр.
- Добавлены отдельные reusable meshes для бревна, страницы и креста.
- Добавлена stamina-система для бега.
- Добавлена panic/fear-система: взгляд на сущности, темнота и преследователь усиливают панику.
- Добавлена батарея фонаря и управление F / LIGHT.
- День 6 получил grace-period и адаптивного преследователя.
- Крест на Дне 5 теперь механически отталкивает силуэты.
- Добавлен HUD-трекер расстояния до текущей цели.
- Touch controls теперь масштабируются под фактический размер окна/экрана.
- Позиции деревьев предвычисляются один раз, а не регенерируются каждый кадр.
- Улучшен рендер прозрачных billboards: depth write отключается на время alpha pass.
- Исправлена переносимость `M_PI` для Windows/MSVC.
- Улучшен CMake: системный SDL2, fallback FetchContent, предупреждения компилятора.
- Улучшена очистка shader resources при ошибках компиляции/линковки.

## Архитектура

```text
src/main.cpp          SDL/OpenGL bootstrap и главный цикл
src/game.*            состояния игры, дни, цели, сущности, рендер сцены
src/renderer.*        Shader, Mesh, Texture wrappers
src/procedural.*      генерация terrain/cabin/tree/well/car/items
src/camera.*          FPS camera
src/ui.*              2D HUD, bitmap font, mobile controls
third_party/glad      OpenGL loader для desktop
android/              Android Gradle skeleton
```
