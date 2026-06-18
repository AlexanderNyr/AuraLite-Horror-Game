#pragma once
#include <string>

struct SaveData {
    int currentDay = 1;
    bool wellRepaired = false;
    int logsCollected = 0;
    int flowersCollected = 0;
    int stonesCollected = 0;
    bool toolFound = false;
    bool herbFound = false;
};

class SaveSystem {
public:
    static bool saveGame(const SaveData& data, const std::string& filePath = "anxietyhorror_save.dat");
    static bool loadGame(SaveData& data, const std::string& filePath = "anxietyhorror_save.dat");
    static bool hasSaveGame(const std::string& filePath = "anxietyhorror_save.dat");
};
