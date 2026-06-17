# Localization with `.lang` files

AuraLite now loads all visible gameplay text from external `.lang` files.

## Files

```text
lang/en.lang
lang/ru.lang
lang/es.lang
```

## Format

```ini
# comments start with #
key = value
multiline.key = First line\nSecond line
```

Supported escapes:

- `\n` newline
- `\t` tab
- `\r` carriage return
- `\\` backslash
- `\#` literal hash
- `\=` literal equals

## Runtime controls

- `F2` cycles language in-game.
- `AURALITE_LANG=ru` / `en` / `es` chooses startup language.
- If `AURALITE_LANG` is not set, the game checks the OS `LANG` environment variable.
- Missing translation keys fall back to English, then to the hardcoded fallback string.
- Desktop builds copy `lang/` next to the executable via CMake.
- Android packages `lang/` as assets and reads them through SDL_RWFromFile.

## Adding a new language

1. Copy `lang/en.lang` to a new file, for example `lang/de.lang`.
2. Translate values after `=`.
3. Add the language to `Localization::init()` in `src/localization.cpp`:

```cpp
languages = {
    {"en", "English", langDir + "/en.lang"},
    {"ru", "Russkiy", langDir + "/ru.lang"},
    {"es", "Espanol", langDir + "/es.lang"},
    {"de", "Deutsch", langDir + "/de.lang"}
};
```

## Font note

The UI renderer uses the original 8x8 ASCII atlas for Latin text and a compact built-in 5x7 pixel font for Cyrillic UTF-8 text. Russian `.lang` files can therefore use real Cyrillic text.
