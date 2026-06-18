# Обзор проекта AuraLite / AnxietyHorror

Репозиторий изучен локально: `AuraLite-Horror-Game`.

## 1. Назначение проекта

Это C++17/SDL2/OpenGL хоррор-игра от первого лица под названием `AnxietyHorror`: девять игровых дней в открытой туманной долине. Игра процедурно генерирует мир, объекты, материалы, погоду, звук и UI без внешних ассетов кроме файлов локализации.

Основные платформы:

- Desktop / Windows / Linux: SDL2 + OpenGL core 4.5 + GLAD.
- Android: SDL2 + OpenGL ES 3.2 через NDK/CMake/Gradle.

## 2. Структура проекта

```text
CMakeLists.txt                 # Главная CMake-сборка desktop/Android
build_windows.bat              # Windows-скрипт сборки
android/                       # Android Gradle-проект
lang/                          # Локализация en/ru/es
src/                           # Исходники игры
third_party/glad/              # GLAD для desktop OpenGL
anxietyhorror_save.dat         # Текущий сохранённый прогресс, попал в репозиторий
anxietyhorror_settings.dat     # Текущие настройки, попали в репозиторий
```

Основные модули `src/`:

- `main.cpp` — запуск SDL, окно, GL-контекст, главный цикл.
- `game.hpp/cpp` — ядро игры: состояния, мир, задачи, враги, погода, рендер, меню.
- `renderer.hpp/cpp` — шейдеры, текстуры, mesh upload/draw, shadow map.
- `procedural.hpp/cpp` — процедурная геометрия: terrain, cabin, trees, well, car, houses, rocks, fences, path.
- `camera.hpp/cpp` — FPS-камера.
- `ui.hpp/cpp` — 2D UI, прямоугольники, bitmap-шрифт ASCII + минимальный кириллический шрифт.
- `audio.hpp/cpp` — процедурный аудиосинтез через SDL audio callback.
- `localization.hpp/cpp` — чтение `.lang`, fallback, выбор языка.
- `save_system.hpp/cpp` — сохранение прогресса.
- `settings_system.hpp/cpp` — сохранение настроек.
- `math3d.hpp` — Vec2, Vec3, Mat4, AABB, Frustum.

## 3. Игровой цикл и состояния

Состояния игры:

- `STATE_MENU` — главное меню.
- `STATE_MENU_SETTINGS` — настройки.
- `STATE_INTRO` — дневниковый экран перед днём.
- `STATE_GAMEPLAY` — игровой режим.
- `STATE_SLEEP_FADE` — переход ко следующему дню.
- `STATE_ENDING` — концовка или game over.

`main.cpp` создаёт окно 1280x720, включает OpenGL, запускает `game.init()`, `audio.init()`, затем каждый кадр:

1. Обрабатывает SDL events.
2. Передаёт события в `Game::handleEvent()`.
3. Считает delta time.
4. Вызывает `game.update(deltaTime)`.
5. Вызывает `game.render()`.
6. Меняет буферы через `SDL_GL_SwapWindow()`.

Escape на desktop переключает relative mouse mode, на Android выходит.

## 4. Геймплей

Игра построена вокруг 9 дней. Каждый день задаёт атмосферу и задачу:

1. Исследовать долину и деревню.
2. Проверить старый колодец.
3. Собрать 3 полена.
4. Собрать 8 цветов.
5. Найти старый инструмент в западной части.
6. Собрать 6 камней.
7. Найти редкие травы.
8. Закончить подготовку.
9. Вернуться к домику и завершить историю.

Механики:

- WASD — движение.
- Мышь — обзор.
- Shift — спринт.
- E / Enter / action — взаимодействие.
- Фонарик появляется с поздних дней.
- Выносливость уменьшается при спринте, усталость накапливается.
- С 3-го дня появляются враги/тени, повышающие `fear`.
- При `fear >= 1.0` происходит game over.

## 5. Мир и процедурная генерация

Мир — открытая долина с terrain mesh, деревьями, деревней и объектами.

`procedural.cpp` генерирует:

- Terrain через сетку и функцию высоты `getTerrainHeight()`.
- Box/cylinder/cone primitives.
- Cabin, tree, well, car.
- House, rock, fence, path.

`Game::generateVillage()` создаёт набор `VillageObject`: дома, камни, заборы, колодец. `Game::buildColliders()` создаёт AABB и круговые коллайдеры для деревьев, домов, забора, колодца, автомобиля и других препятствий.

Для оптимизации используется frustum culling (`Frustum` из `math3d.hpp`) при рендере деревьев/объектов.

## 6. Рендеринг

Рендеринг сделан напрямую на OpenGL:

- Desktop: `#version 450 core`.
- Android: `#version 320 es`.

Основной 3D shader включает:

- Cook-Torrance BRDF.
- Hemispheric ambient approximation.
- Процедурный detail normal.
- Directional light.
- Flashlight spotlight.
- Fog / procedural noise fog.
- ACES tonemapping.
- Shadow mapping с `sampler2DShadow` и 3x3 PCF.

Есть отдельные shader-проходы:

- `mainShader` — основная геометрия.
- `billboardShader` — billboard-объекты: цветы, предметы, враги/призраки.
- `depthShader` — depth pass для shadow map.
- `skyShader` — процедурное небо.

Текстуры полностью процедурные:

- grass/moss ground.
- weathered wood.
- path.
- stone/rock.
- bark.

`ShadowMap` создаёт depth FBO 2048x2048.

## 7. Время суток и погода

`timeOfDay` изменяется со временем. `applyTimeOfDay()` рассчитывает:

- положение солнца и луны;
- `nightFactor`;
- ambient/direct/fog colors;
- sky zenith/horizon colors;
- силу света и тумана.

Погода зависит от дня:

- Дни 1-2: clear.
- Дни 3-4: foggy или clear.
- Дни 5-6: rain/foggy/clear.
- Дни 7-9: snow/rain/foggy.

Для дождя и снега есть массивы частиц и mesh-представление.

## 8. Аудио

Звук полностью процедурный, через `SDL_OpenAudioDevice` и callback 44100 Hz mono S16.

Слои аудио:

- spooky wind / fog drone;
- low hum, усиливающийся с днём;
- footsteps;
- heartbeat при усталости/низкой stamina;
- one-shot SFX: pickup, sleep, repair, intro.

Состояние аудио обновляется из игры через `audio.update(fogDensity, currentDay, moving, sprinting, stamina, fatigue)`.

## 9. UI и локализация

UI рендерится собственным `UIRenderer`:

- цветные прямоугольники;
- bitmap ASCII font в текстуре;
- минимальный 5x7 кириллический шрифт, рисуемый прямоугольниками;
- виртуальные джойстики на Android.

Локализация в `lang/en.lang`, `lang/ru.lang`, `lang/es.lang`.

Формат простой:

```text
key = value
```

Поддерживаются escape-последовательности `\n`, `\t`, `\r`, `\\`, `\#`, `\=`.

На desktop файлы ищутся в `lang`, `../lang`, `../../lang`, `../../../lang`; на Android читаются через `SDL_RWFromFile` из assets.

## 10. Сохранения и настройки

Сохранение прогресса: `anxietyhorror_save.dat`.

Поля:

- `currentDay`
- `wellRepaired`
- `logsCollected`
- `flowersCollected`
- `stonesCollected`
- `toolFound`
- `herbFound`

Настройки: `anxietyhorror_settings.dat`.

Поля:

- `languageIndex` — 0 en, 1 ru, 2 es.
- `mouseSensitivity` — clamp 0.001..0.03.
- `viewDistance` — clamp 0.5..2.0.

На Android пути сохраняются во внутреннее хранилище через `SDL_AndroidGetInternalStoragePath()`.

## 11. Сборка

Desktop:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Windows также имеет `build_windows.bat`, который запускает CMake и копирует `AnxietyHorror.exe` в корень.

Android:

```bash
cd android
./gradlew assembleDebug
```

Но в репозитории нет Gradle wrapper-файлов (`gradlew`, `gradle/wrapper/...`), поэтому команда из README сработает только если wrapper добавить или использовать установленный `gradle`.

## 12. Что удалось проверить в текущей среде

Репозиторий успешно склонирован. Полная сборка не выполнена, потому что в среде отсутствует `cmake`:

```text
/bin/bash: line 1: cmake: command not found
```

Статический разбор файлов выполнен.

## 13. Замечания и потенциальные проблемы

1. В репозиторий попали пользовательские файлы:
   - `anxietyhorror_save.dat`
   - `anxietyhorror_settings.dat`
   - `.sudo_as_admin_successful`

   Обычно их стоит удалить из репозитория и добавить save/settings в `.gitignore`.

2. `.gitignore` не игнорирует `anxietyhorror_save.dat` и `anxietyhorror_settings.dat`.

3. В Android README указано `./gradlew assembleDebug`, но Gradle wrapper отсутствует.

4. CMake message пишет `OpenGL 4.6`, а `main.cpp` реально запрашивает OpenGL 4.5. Это не критично, но вводит в заблуждение.

5. В `CMakeLists.txt` для Android `target_include_directories(main PRIVATE "${SDL2_SOURCE_DIR}/include")` зависит от корректного значения `SDL2_SOURCE_DIR`; при FetchContent оно выставляется через `sdl2_SOURCE_DIR`, что обычно работает, но стоит проверить на чистой Android-сборке.

6. Android использует AppCompat-зависимости ради темы, хотя игра на SDL может обойтись более лёгкой Activity/theme. Не ошибка, но усложняет зависимости.

7. Все ассеты процедурные — это плюс для размера, но усложняет художественную доработку: нет пайплайна внешних моделей/текстур.

8. `saveProgress()` сохраняет только счётчики, не позицию игрока, время дня, страх, батарею и т.п. Это соответствует текущей структуре дней, но если нужен полноценный continue «с места», систему придётся расширять.

9. Проверить компиляцию shader-кода на реальных GPU обязательно: проект использует достаточно сложные GLSL-функции, depth compare sampler и ES/Desktop ветвление.

## 14. Общая оценка архитектуры

Проект компактный и монолитный: большая часть логики находится в `game.cpp` (~1900 строк). Это удобно для прототипа, но при развитии лучше разделить:

- `World/Environment`
- `QuestSystem`
- `EnemySystem`
- `WeatherSystem`
- `RenderScene`
- `InputSystem`
- `Menu/UI states`

Сильные стороны:

- Минимум внешних ассетов.
- Есть desktop и Android таргеты.
- Есть локализация.
- Есть save/settings.
- Есть процедурные текстуры, погода, звук, тени.
- Хороший объём готового gameplay loop.

Слабые стороны:

- Монолитный `Game`.
- Нет автоматических тестов.
- Нет CI.
- Нет Gradle wrapper.
- В репозиторий попали локальные save/settings.
- Нужна реальная сборка/прогон на Windows/Linux/Android.
