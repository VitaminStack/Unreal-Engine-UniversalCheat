#pragma once
#include <Windows.h>
#include <string>
#include <map>
#include <fstream>

class ConfigManager {
public:
    ConfigManager(const std::string& filename = "Config.ini")
        : iniFile(filename)
    {
        if (!FileExists(iniFile)) {
            GenerateDefaultConfig();
        }
    }

    // Liest einen Hotkey, z. B. "Flyhack" => "VK_F"
    std::string GetHotkey(const std::string& cheatName, const std::string& defaultKey = "") {
        char buffer[128] = { 0 };
        GetPrivateProfileStringA("Hotkeys", cheatName.c_str(), defaultKey.c_str(), buffer, sizeof(buffer), iniFile.c_str());
        return std::string(buffer);
    }

    // Setzt einen Hotkey, z. B. "Flyhack", "VK_F2"
    void SetHotkey(const std::string& cheatName, const std::string& key) {
        WritePrivateProfileStringA("Hotkeys", cheatName.c_str(), key.c_str(), iniFile.c_str());
    }

    // Für beliebige Sektionen/Keys
    std::string GetValue(const std::string& section, const std::string& key, const std::string& defaultValue = "") {
        char buffer[128] = { 0 };
        GetPrivateProfileStringA(section.c_str(), key.c_str(), defaultValue.c_str(), buffer, sizeof(buffer), iniFile.c_str());
        return std::string(buffer);
    }

    void SetValue(const std::string& section, const std::string& key, const std::string& value) {
        WritePrivateProfileStringA(section.c_str(), key.c_str(), value.c_str(), iniFile.c_str());
    }

    // Standard-Hotkeys beim ersten Start
    void GenerateDefaultConfig() {
        WritePrivateProfileStringA("Hotkeys", "Flyhack", "VK_F", iniFile.c_str());
        WritePrivateProfileStringA("Hotkeys", "Godmode", "VK_G", iniFile.c_str());
        WritePrivateProfileStringA("Hotkeys", "ESP", "VK_E", iniFile.c_str());
    }

private:
    std::string iniFile;

    bool FileExists(const std::string& file) {
        std::ifstream f(file.c_str());
        return f.good();
    }
};
