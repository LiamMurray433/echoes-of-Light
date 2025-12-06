#pragma once
#include <string>

class SettingsManager {
public:
    static SettingsManager& get() {
        static SettingsManager instance;
        return instance;
    }

    void load();
    void save() const;

    // Settings you want to store
    unsigned int framerate = 60;
    unsigned int resolutionIndex = 0;
    bool fullscreen = false;

private:
    SettingsManager() = default;
    ~SettingsManager() = default;

    std::string settingsPath = "settings.json";
};