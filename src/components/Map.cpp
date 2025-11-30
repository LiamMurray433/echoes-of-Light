#include "components/Map.h"
#include <fstream>
#include <iostream>

TileType Map::charToTileType(char c) const {
    switch (c) {
        case '#': return TileType::WALL;
        case 'L': return TileType::LIGHT_SOURCE;
        case '/': return TileType::MIRROR;
        case 'S': return TileType::START;
        case 'E': return TileType::END;
        default:  return TileType::EMPTY;
    }
}

bool Map::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to load map: " << filename << std::endl;
        return false;
    }

    grid.clear();
    std::string line;

    while (std::getline(file, line)) {
        std::vector<TileType> row;
        for (char c : line) {
            row.push_back(charToTile(c));
        }
        grid.push_back(row);
    }

    height = grid.size();
    width = height > 0 ? grid[0].size() : 0;

    std::cout << "Map loaded: " << width << "x" << height << std::endl;
    return true;
}

TileType Map::getTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return TileType::WALL; // Out of bounds treated as wall
    }
    return grid[y][x];
}

window.clear();
levels.getMap().draw(window);
window.display();

#include <SFML/Graphics.hpp>
class Map{
    public:
    bool loadFromFile(const std::string& filename);
    TileType getTile(int x, int y) const;
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    void setTextures(const sf::Texture& wallTex,
                     const sf::Texture& lightTex,
                     const sf::Texture& mirrorTex,
                     const sf::Texture& startTex,
                     const sf::Texture& endTex,
                     const sf::Texture& emptyTex);
    void draw(sf::RenderWindow& window) const;
    private:
    std::vector<std::vector<TileType>> grid;
    int width = 0;
    int height = 0;

    const sf::Texture* wallTexture = nullptr;
    const sf::Texture* lightTexture = nullptr;
    const sf::Texture* mirrorTexture = nullptr;
    const sf::Texture* exitTexture = nullptr;
    const sf::Texture* floorTexture = nullptr;

    TileType charToTile(char c) const;
};

void Map::setTextures(const sf::Texture& wallTex,
                      const sf::Texture& lightTex,
                      const sf::Texture& mirrorTex,
                      const sf::Texture& startTex,
                      const sf::Texture& endTex,
                      const sf::Texture& emptyTex) {
    wallTexture = &wallTex;
    lightTexture = &lightTex;
    mirrorTexture = &mirrorTex;
    exitTexture = &endTex;
    floorTexture = &emptyTex;
}

void Map::draw(sf::RenderWindow& window) const {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            sf::Sprite sprite;
            switch (getTile(x, y)) {
                case TileType::WALL:
                    if (wallTexture) sprite.setTexture(*wallTexture);
                    break;
                case TileType::LIGHT_SOURCE:
                    if (lightTexture) sprite.setTexture(*lightTexture);
                    break;
                case TileType::MIRROR:
                    if (mirrorTexture) sprite.setTexture(*mirrorTexture);
                    break;
                case TileType::START:
                    if (floorTexture) sprite.setTexture(*floorTexture);
                    break;
                case TileType::END:
                    if (exitTexture) sprite.setTexture(*exitTexture);
                    break;
                case TileType::EMPTY:
                    if (floorTexture) sprite.setTexture(*floorTexture);
                    break;
            }

            if (tex){
            sprite.setTexture(*tex);
            sprite.setPosition(x * TILE_SIZE, y * TILE_SIZE);
            window.draw(sprite);
        }
    }
}

sf::Texture wallTex, lightTex, mirrorTex, startTex, exitTex;
wallTex.loadFromFile("resources/textures/wall.png");
lightTex.loadFromFile("resources/textures/light_source.png");
mirrorTex.loadFromFile("resources/textures/mirror.png");
exitTex.loadFromFile("resources/textures/exit.png");
floorTex.loadFromFile("resources/textures/floor.png");

levels.loadCurrentLevel();
levels.getMap().setTextures(wallTex, lightTex, mirrorTex, exitTex, floorTex);

bool Map::isWalkableTile(TileType t) const {
    switch (t) {
        case TileType::WALL:
            return false;
        default:
            return true;
    }
}

bool Map::isWalkableTileCoord(int tx, int ty) const {
    if (tx < 0 || tx >= width || ty < 0 || ty >= height) 
        return false;
    return isWalkableTile(getTile(tx, ty));
}

bool Map::isWalkableWorld(float worldx, float worldy) const {
    int tx = static_cast<int>(worldx) / TILE_SIZE;
    int ty = static_cast<int>(worldy) / TILE_SIZE;
    return isWalkableTileCoord(tx, ty);
}

sf::Vector2f newPos = player.getPosition() + moveDelta;
if (map.isWalkableWorld(newPos.x, newPos.y)) {
    player.setPosition(newPos);
}