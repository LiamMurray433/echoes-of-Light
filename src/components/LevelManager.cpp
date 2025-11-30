#include "components/LevelManager.h"
#include <iostream>


// Constructor
LevelManager::LevelManager()
    : currentLevelIndex(0),
      map("resources/levels/past.txt")   // default temp, replaced on load
{
    // Predefined level files
    levelFiles = {
        "resources/levels/past.txt",
        "resources/levels/present.txt",
        "resources/levels/future.txt"
    };
}

// Load a specific level by filename
bool LevelManager::loadLevel(const std::string& levelName) {
    std::cout << "Loading level: " << levelName << std::endl;
    return map.loadFromFile(levelName);
}

// Load the current level in the list
bool LevelManager::loadCurrentLevel() {
    if (currentLevelIndex >= levelFiles.size()) {
        std::cout << "No more levels to load." << std::endl;
        return false;
    }

    const std::string& filename = levelFiles[currentLevelIndex];

    std::cout << "Loading level " << (currentLevelIndex + 1)
              << " / " << levelFiles.size()
              << " : " << filename << std::endl;

    return map.loadFromFile(filename);
}

// Advance to the next level
void LevelManager::nextLevel() {
    if (currentLevelIndex < levelFiles.size()) {
        currentLevelIndex++;
    }

    if (!isLevelComplete()) {
        loadCurrentLevel();
    }
}


// Check if all levels are finished
bool LevelManager::isLevelComplete() const {
    return currentLevelIndex >= levelFiles.size();
}

// Return the map reference
const Map& LevelManager::getCurrentMap() const {
    return map;
}

// SCAN MAP FOR LIGHTS, MIRRORS, EXIT

LevelObjects LevelManager::scanObjects() const {
    LevelObjects objs;

    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {

            TileType t = map.getTile(x, y);

            switch (t) {
                case TileType::LIGHT_SOURCE:
                    objs.lightTiles.emplace_back(x, y);
                    break;

                case TileType::MIRROR:
                    objs.mirrorTiles.emplace_back(x, y);
                    break;

                case TileType::END:
                    objs.exitTile = {x, y};
                    break;

                default:
                    break;
            }
        }
    }

    return objs;
}
