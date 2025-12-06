#include "SettingsManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

void SettingsManager::load() {
    std::ifstream file(settingsPath);
    if (!file.is_open()) {
        std::cout << "Settings: no file found, using defaults.\n";
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string data = buffer.str();
    file.close();

    // Very small JSON-like parser (no libraries)
    if (data.find("framerate") != std::string::npos)
        framerate = std::stoi(data.substr(data.find("framerate") + 11));

    if (data.find("resolution") != std::string::npos)
        resolutionIndex = std::stoi(data.substr(data.find("resolution") + 12));

    if (data.find("fullscreen") != std::string::npos)
        fullscreen = (data.substr(data.find("fullscreen") + 12)[0] == '1');

    std::cout << "Settings loaded.\n";
}

void SettingsManager::save() const {
    std::ofstream file(settingsPath);
    if (!file.is_open()) {
        std::cerr << "Settings: failed to save\n";
        return;
    }

    file << "{\n";
    file << "  \"framerate\": " << framerate << ",\n";
    file << "  \"resolution\": " << resolutionIndex << ",\n";
    file << "  \"fullscreen\": " << (fullscreen ? 1 : 0) << "\n";
    file << "}\n";

    file.close();
    std::cout << "Settings saved.\n";
}
