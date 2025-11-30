#pragma once
#include <string>
#include <vector>
#include <SFML/System/Vector2.hpp>

#include "components/Map.h"

// Struct that stores all special tile positions for a level
struct LevelObjects {
    std::vector<sf::Vector2i> lightTiles;
    std::vector<sf::Vector2i> mirrorTiles;
    sf::Vector2i exitTile { -1, -1 };
};

class LevelManager {

public:
    LevelManager();
    ~LevelManager() = default;

    bool loadLevel(const std::string& levelName);
    bool loadCurrentLevel();
    void nextLevel();

    bool isLevelComplete() const;
    const Map& getCurrentMap() const;

    LevelObjects scanObjects() const;

private:
    std::vector<std::string> levelFiles;
    int currentLevelIndex = 0;

    // Active map instance
    Map map;
};
