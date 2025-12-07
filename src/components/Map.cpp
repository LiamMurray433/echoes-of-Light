#include "components/Map.h"
#include <fstream>
#include <iostream>

TileType Map::charToTile(char c) const {
    switch (c) {
        case '#': return TileType::WALL;
        case 'L': return TileType::LIGHT_SOURCE;
        case '/': return TileType::MIRROR;
        case '1': return TileType::BEACON_1;
        case '2': return TileType::BEACON_2;
        case '3': return TileType::BEACON_3;
        case '4': return TileType::BEACON_4;
        case 'S': return TileType::START;
        case 'E': return TileType::END;
        case 'X': return TileType::SPAWNER;
        default:  return TileType::EMPTY;
    }
}

bool Map::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Map::loadFromFile - failed to open: " << filename << std::endl;
        return false;
    }

    grid.clear();
    std::string line;
    while (std::getline(file, line)) {
        std::vector<TileType> row;
        row.reserve(line.size());
        for (char c : line) row.push_back(charToTile(c));
        grid.push_back(std::move(row));
    }

    height = static_cast<int>(grid.size());
    width = height > 0 ? static_cast<int>(grid[0].size()) : 0;

    std::cout << "Map loaded: " << filename << " (" << width << "x" << height << ")\n";
    return true;
}

TileType Map::getTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return TileType::WALL;
    return grid[y][x];
}

void Map::setTextures(const sf::Texture& wallTex,
                      const sf::Texture& lightTex,
                      const sf::Texture& mirrorTex,
                      const sf::Texture& startTex,
                      const sf::Texture& endTex,
                      const sf::Texture& emptyTex) {
    wallTexture = &wallTex;
    lightTexture = &lightTex;
    mirrorTexture = &mirrorTex;
    startTexture = &startTex;
    exitTexture = &endTex;
    emptyTexture = &emptyTex;
}


void Map::draw(sf::RenderWindow& window, float tileSize, sf::Vector2f offset) const {
    if (width == 0 || height == 0) return;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            TileType tile = getTile(x, y);
            sf::Vector2f tilePos(offset.x + x * tileSize, offset.y + y * tileSize);

            // Check if we have a texture for this tile type
            const sf::Texture* tex = nullptr;
            sf::Color fallbackColor;

            switch (tile) {
            case TileType::WALL:
                tex = wallTexture;
                fallbackColor = sf::Color(80, 80, 100);
                break;
            case TileType::LIGHT_SOURCE:
                tex = lightTexture;
                fallbackColor = sf::Color(255, 255, 150);
                break;
            case TileType::MIRROR:
                tex = emptyTexture;
                fallbackColor = sf::Color(30, 30, 40);
                break;
            case TileType::START:
                tex = startTexture;
                fallbackColor = sf::Color(100, 255, 100);
                break;
            case TileType::END:
                tex = exitTexture;
                fallbackColor = sf::Color(255, 100, 100);
                break;
            case TileType::EMPTY:
            default:
                tex = emptyTexture;
                fallbackColor = sf::Color(30, 30, 40);
                break;
            }

            if (tex) {
                sf::Sprite sprite(*tex);
                sf::Vector2u texSize = tex->getSize();
                float scaleX = tileSize / static_cast<float>(texSize.x);
                float scaleY = tileSize / static_cast<float>(texSize.y);
                sprite.setScale(sf::Vector2f(scaleX, scaleY));
                sprite.setPosition(tilePos);
                window.draw(sprite);
            }
            else {
                sf::RectangleShape rect;
                rect.setSize(sf::Vector2f(tileSize, tileSize));
                rect.setPosition(tilePos);
                rect.setFillColor(fallbackColor);
                window.draw(rect);
            }
        }
    }
}

bool Map::isWalkableTile(TileType t) const {
    switch (t) {
        case TileType::WALL: return false;
        default: return true;
    }
}

bool Map::isWalkableTileCoord(int tx, int ty) const {
    if (tx < 0 || tx >= width || ty < 0 || ty >= height) return false;
    return isWalkableTile(getTile(tx, ty));
}

bool Map::isWalkableWorld(float worldx, float worldy, float tileSize) const {
    int tx = static_cast<int>(worldx) / tileSize;
    int ty = static_cast<int>(worldy) / tileSize;
    return isWalkableTileCoord(tx, ty);
}

