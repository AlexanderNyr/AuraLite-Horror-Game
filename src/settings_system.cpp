#include "settings_system.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

#ifdef __ANDROID__
#include <SDL2/SDL.h>
#endif

static std::string resolveSettingsPath(const std::string& filePath) {
#ifdef __ANDROID__
    if (const char* internalPath = SDL_AndroidGetInternalStoragePath()) {
        return std::string(internalPath) + "/" + filePath;
    }
#endif
    return filePath;
}

void GameSettings::sanitize() {
    if (languageIndex < 0) languageIndex = 0;
    if (languageIndex > 2) languageIndex = 2;
    if (mouseSensitivity < 0.001f) mouseSensitivity = 0.001f;
    if (mouseSensitivity > 0.03f) mouseSensitivity = 0.03f;
    if (viewDistance < 0.5f) viewDistance = 0.5f;
    if (viewDistance > 2.0f) viewDistance = 2.0f;
}

bool SettingsSystem::saveSettings(const GameSettings& settings, const std::string& filePath) {
    GameSettings s = settings;
    s.sanitize();
    std::ofstream out(resolveSettingsPath(filePath));
    if (!out.is_open()) {
        std::cerr << "Warning: Failed to open settings file for writing: " << filePath << std::endl;
        return false;
    }
    out << "languageIndex=" << s.languageIndex << "\n";
    out << "mouseSensitivity=" << s.mouseSensitivity << "\n";
    out << "viewDistance=" << s.viewDistance << "\n";
    out.close();
    return true;
}

bool SettingsSystem::loadSettings(GameSettings& settings, const std::string& filePath) {
    std::ifstream file(resolveSettingsPath(filePath));
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        try {
            if (key == "languageIndex") settings.languageIndex = std::stoi(val);
            else if (key == "mouseSensitivity") settings.mouseSensitivity = std::stof(val);
            else if (key == "viewDistance") settings.viewDistance = std::stof(val);
        } catch (...) {
            // Ignore parsing errors on corrupted fields
        }
    }
    file.close();
    settings.sanitize();
    return true;
}

bool SettingsSystem::hasSettings(const std::string& filePath) {
    std::ifstream file(resolveSettingsPath(filePath));
    return file.is_open();
}
