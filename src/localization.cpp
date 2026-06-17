#include "localization.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#ifdef __ANDROID__
#include <SDL2/SDL.h>
#endif

static std::string trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace((unsigned char)value[start])) ++start;

    size_t end = value.size();
    while (end > start && std::isspace((unsigned char)value[end - 1])) --end;

    return value.substr(start, end - start);
}

static std::string unescape(const std::string& value) {
    std::string result;
    result.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 1 < value.size()) {
            char next = value[++i];
            switch (next) {
                case 'n': result.push_back('\n'); break;
                case 't': result.push_back('\t'); break;
                case 'r': result.push_back('\r'); break;
                case '\\': result.push_back('\\'); break;
                case '#': result.push_back('#'); break;
                case '=': result.push_back('='); break;
                default:
                    result.push_back(next);
                    break;
            }
        } else {
            result.push_back(value[i]);
        }
    }

    return result;
}

void Localization::init(const std::string& languageDirectory) {
    langDir = languageDirectory;

    // The game intentionally uses simple external .lang files so translations can be
    // edited without recompiling. Keep this list explicit to work in tiny builds where
    // filesystem directory iteration may be unavailable or restricted.
    languages = {
        {"en", "English", langDir + "/en.lang"},
        {"ru", "Русский", langDir + "/ru.lang"},
        {"es", "Espanol", langDir + "/es.lang"}
    };

    fallbackTable.clear();
    loadFile(languages[0].filePath, fallbackTable);

    std::string preferred = "en";
    if (const char* envLang = std::getenv("AURALITE_LANG")) {
        preferred = envLang;
    } else if (const char* systemLang = std::getenv("LANG")) {
        std::string lang = systemLang;
        std::transform(lang.begin(), lang.end(), lang.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (lang.rfind("ru", 0) == 0) preferred = "ru";
        else if (lang.rfind("es", 0) == 0) preferred = "es";
    }

    if (!loadLanguage(preferred)) {
        loadLanguage("en");
    }
}

bool Localization::loadLanguage(const std::string& code) {
    int idx = findLanguageIndex(code);
    if (idx < 0) return false;

    std::unordered_map<std::string, std::string> loaded;
    if (!loadFile(languages[idx].filePath, loaded)) {
        return false;
    }

    table = std::move(loaded);
    currentIndex = idx;
    return true;
}

void Localization::cycleLanguage() {
    if (languages.empty()) return;

    for (size_t step = 1; step <= languages.size(); ++step) {
        int nextIndex = (currentIndex + (int)step) % (int)languages.size();
        if (loadLanguage(languages[nextIndex].code)) {
            return;
        }
    }
}

const std::string& Localization::currentCode() const {
    static const std::string empty = "en";
    if (currentIndex >= 0 && currentIndex < (int)languages.size()) return languages[currentIndex].code;
    return empty;
}

const std::string& Localization::currentName() const {
    static const std::string empty = "English";
    if (currentIndex >= 0 && currentIndex < (int)languages.size()) return languages[currentIndex].name;
    return empty;
}

std::string Localization::tr(const std::string& key, const std::string& fallback) const {
    auto it = table.find(key);
    if (it != table.end()) return it->second;

    auto fb = fallbackTable.find(key);
    if (fb != fallbackTable.end()) return fb->second;

    return fallback.empty() ? key : fallback;
}

bool Localization::loadFile(const std::string& path, std::unordered_map<std::string, std::string>& outTable) const {
    auto parseStream = [&](std::istream& input) {
        std::string line;
        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();

            std::string stripped = trim(line);
            if (stripped.empty() || stripped[0] == '#') continue;

            size_t eq = stripped.find('=');
            if (eq == std::string::npos) continue;

            std::string key = trim(stripped.substr(0, eq));
            std::string value = trim(stripped.substr(eq + 1));
            if (key.empty()) continue;

            outTable[key] = unescape(value);
        }
    };

#ifdef __ANDROID__
    // SDL_RWFromFile can read packaged Android assets, unlike std::ifstream.
    if (SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "rb")) {
        Sint64 size = SDL_RWsize(rw);
        if (size > 0) {
            std::vector<char> buffer((size_t)size);
            SDL_RWread(rw, buffer.data(), 1, (size_t)size);
            SDL_RWclose(rw);
            std::string text(buffer.begin(), buffer.end());
            std::istringstream stream(text);
            parseStream(stream);
            return true;
        }
        SDL_RWclose(rw);
    }
#endif

    // Desktop fallback: support running from project root, build/, build/Release,
    // or a manually copied executable without breaking localization.
    const std::vector<std::string> candidates = {
        path,
        "../" + path,
        "../../" + path,
        "../../../" + path
    };

    for (const std::string& candidate : candidates) {
        std::ifstream file(candidate);
        if (file.is_open()) {
            parseStream(file);
            return true;
        }
    }

    return false;
}

int Localization::findLanguageIndex(const std::string& code) const {
    std::string normalized = code;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) { return (char)std::tolower(c); });

    // Accept values like ru_RU.UTF-8 or es-ES.
    size_t sep = normalized.find_first_of("_.-");
    if (sep != std::string::npos) normalized = normalized.substr(0, sep);

    for (int i = 0; i < (int)languages.size(); ++i) {
        if (languages[i].code == normalized) return i;
    }
    return -1;
}
