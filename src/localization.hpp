#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct LanguageInfo {
    std::string code;
    std::string name;
    std::string filePath;
};

class Localization {
public:
    void init(const std::string& languageDirectory = "lang");

    bool loadLanguage(const std::string& code);
    void cycleLanguage();

    const std::string& currentCode() const;
    const std::string& currentName() const;

    std::string tr(const std::string& key, const std::string& fallback = "") const;

private:
    std::string langDir = "lang";
    std::vector<LanguageInfo> languages;
    int currentIndex = 0;

    std::unordered_map<std::string, std::string> fallbackTable;
    std::unordered_map<std::string, std::string> table;

    bool loadFile(const std::string& path, std::unordered_map<std::string, std::string>& outTable) const;
    int findLanguageIndex(const std::string& code) const;
};
