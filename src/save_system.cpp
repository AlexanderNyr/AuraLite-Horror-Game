#include "save_system.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

bool SaveSystem::saveGame(const SaveData& data, const std::string& filePath) {
    std::ofstream out(filePath);
    if (!out.is_open()) {
        std::cerr << "Warning: Failed to open save file for writing: " << filePath << std::endl;
        return false;
    }

    out << "currentDay=" << data.currentDay << "\n";
    out << "wellRepaired=" << (data.wellRepaired ? 1 : 0) << "\n";
    out << "logsCollected=" << data.logsCollected << "\n";
    out << "flowersCollected=" << data.flowersCollected << "\n";
    out << "stonesCollected=" << data.stonesCollected << "\n";
    out << "toolFound=" << (data.toolFound ? 1 : 0) << "\n";
    out << "herbFound=" << (data.herbFound ? 1 : 0) << "\n";

    out.close();
    return true;
}

bool SaveSystem::loadGame(SaveData& data, const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        try {
            if (key == "currentDay") data.currentDay = std::stoi(val);
            else if (key == "wellRepaired") data.wellRepaired = (std::stoi(val) != 0);
            else if (key == "logsCollected") data.logsCollected = std::stoi(val);
            else if (key == "flowersCollected") data.flowersCollected = std::stoi(val);
            else if (key == "stonesCollected") data.stonesCollected = std::stoi(val);
            else if (key == "toolFound") data.toolFound = (std::stoi(val) != 0);
            else if (key == "herbFound") data.herbFound = (std::stoi(val) != 0);
        } catch (...) {
            // Ignore parsing errors on corrupted fields
        }
    }

    file.close();
    return true;
}

bool SaveSystem::hasSaveGame(const std::string& filePath) {
    std::ifstream file(filePath);
    return file.is_open();
}
