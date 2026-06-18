#pragma once
#include <string>

struct GameSettings {
    int languageIndex = 0;    // 0=en, 1=ru, 2=es
    float mouseSensitivity = 0.005f;
    float viewDistance = 1.0f; // multiplier for fogEnd/view range

    void sanitize();
};

class SettingsSystem {
public:
    static bool saveSettings(const GameSettings& settings, const std::string& filePath = "anxietyhorror_settings.dat");
    static bool loadSettings(GameSettings& settings, const std::string& filePath = "anxietyhorror_settings.dat");
    static bool hasSettings(const std::string& filePath = "anxietyhorror_settings.dat");
};
