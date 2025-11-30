#include "Game.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>

#include "components/AnimationComponent.h"
#include "components/EnemyComponent.h"
#include "components/LightComponent.h"
#include "components/LightEmitterComponent.h"
#include "components/MeleeAttackComponent.h"
#include "components/LightSourceComponent.h"
#include "components/MirrorComponent.h"
#include "components/PlayerComponent.h"
#include "components/PuzzleComponent.h"
#include "components/RenderComponent.h"
#include "components/TransformComponent.h"
#include "components/UpgradeComponent.h"

namespace {
    constexpr char windowTitle[] = "Echoes of Light";

sf::Image createSolidImage(unsigned int size, sf::Color color) {
    sf::Image img(sf::Vector2u(size, size), color);
    return img;
}

sf::Image createCircularFalloffImage(unsigned int size,
                                     sf::Color centerColor,
                                     sf::Color edgeColor) {
    sf::Image img(sf::Vector2u(size, size), sf::Color::Transparent);

    const float half = static_cast<float>(size) * 0.5f;
    for (unsigned int y = 0; y < size; ++y) {
        for (unsigned int x = 0; x < size; ++x) {
            const float dx = static_cast<float>(x) - half + 0.5f;
            const float dy = static_cast<float>(y) - half + 0.5f;
            const float dist = std::sqrt(dx * dx + dy * dy);
            const float norm = dist / half;
            if (norm <= 1.f) {
                const float t = std::clamp(norm, 0.f, 1.f);
                const auto mixChannel = [t](std::uint8_t a, std::uint8_t b) {
                    return static_cast<std::uint8_t>(a * (1.f - t) + b * t);
                };

                sf::Color color;
                color.r = mixChannel(centerColor.r, edgeColor.r);
                color.g = mixChannel(centerColor.g, edgeColor.g);
                color.b = mixChannel(centerColor.b, edgeColor.b);
                color.a = mixChannel(centerColor.a, edgeColor.a);
                img.setPixel(sf::Vector2u{x, y}, color);
            }
        }
    }

    return img;
}
}

Game::Game()
    : window_{}
    , clock_{}
    , idleTexture_{}
    , moveTexture_{}
    , initialized_{false}
    , player_{}
    , lightBeacon_{}
    , enemy_{}
    , entities_{}
    , worldObjects_{}
    , combatSystem_{}
    , lightSystem_{combatSystem_} {}

int Game::run() {
    if (!initialized_ && !initialize()) {
        return -1;
    }

    while (window_.isOpen()) {
        float deltaTime = clock_.restart().asSeconds();

        handleEvents();
        update(deltaTime);
        render();
    }

    return 0;
}

bool Game::initialize() {
    window_.create(sf::VideoMode({ windowWidth, windowHeight }), windowTitle);
    window_.setFramerateLimit(framerateLimit);

    

    std::cout << "=== ECHOES OF LIGHT ===" << std::endl;

    if (!loadResources()) {
        return false;
    }

    createEntities();
    clock_.restart();

    std::cout << "Controls: WASD to move, ESC to exit" << std::endl;
    std::cout << "Animation system active!" << std::endl;

    initialized_ = true;
    return true;
}

bool Game::loadResources() {
    const std::string idlePath = findResourcePath("resources/sprites/Character_Idle.png");
    const std::string movePath = findResourcePath("resources/sprites/Character_Move.png");

    if (!idleTexture_.loadFromFile(idlePath)) {
        std::cerr << "ERROR: Failed to load idle texture" << std::endl;
        return false;
    }

    if (!moveTexture_.loadFromFile(movePath)) {
        std::cerr << "ERROR: Failed to load move texture" << std::endl;
        return false;
    }

    if (!debugWhiteTexture_.loadFromImage(createSolidImage(2, sf::Color::White))) {
        std::cerr << "ERROR: Failed to create debug texture" << std::endl;
        return false;
    }

    const sf::Color centerColor(255, 255, 230, 255);
    const sf::Color edgeColor(255, 255, 230, 80);
    if (!lightNodeTexture_.loadFromImage(createCircularFalloffImage(64, centerColor, edgeColor))) {
        std::cerr << "ERROR: Failed to create light node texture" << std::endl;
        return false;
    }

    std::cout << "Textures loaded successfully" << std::endl;
    return true;
}

void Game::createEntities() {
    player_ = createPlayerEntity();
    lightBeacon_ = createLightBeaconEntity();
    enemy_ = createEnemyEntity();

    entities_.clear();
    entities_.push_back(&player_);
    entities_.push_back(&lightBeacon_);
    entities_.push_back(&enemy_);

    worldObjects_.clear();
    worldObjects_.reserve(4);

    auto addWorldEntity = [this](Entity&& entity) {
        auto handle = std::make_unique<Entity>();
        *handle = std::move(entity);
        entities_.push_back(handle.get());
        worldObjects_.push_back(std::move(handle));
    };

    addWorldEntity(createMirrorEntity(sf::Vector2f{520.f, 280.f},
                                      sf::Vector2f{-1.f, 1.f},
                                      sf::Vector2f{68.f, 18.f},
                                      eol::MirrorComponent::MirrorType::Prism));
    addWorldEntity(createMirrorEntity(sf::Vector2f{360.f, 180.f},
                                      sf::Vector2f{1.f, 1.f},
                                      sf::Vector2f{50.f, 16.f},
                                      eol::MirrorComponent::MirrorType::Splitter));
    addWorldEntity(createMirrorEntity(sf::Vector2f{400.f, 300.f},
                                        sf::Vector2f{1.f, 1.f},
                                        sf::Vector2f{50.f, 16.f},
                                        eol::MirrorComponent::MirrorType::Flat));
    addWorldEntity(createLightSourceNode("PrismNode", sf::Vector2f{260.f, 360.f}, true));
    addWorldEntity(createLightSourceNode("AccessLight", sf::Vector2f{700.f, 420.f}, false));
}

Entity Game::createPlayerEntity() {
    Entity entity;
    entity.name = "Player";

    entity.components.emplace_back(std::make_unique<eol::TransformComponent>(
        sf::Vector2f{ 400.f, 300.f },
        sf::Vector2f{ 0.5f, 0.5f },
        0.f));

    entity.components.emplace_back(std::make_unique<eol::RenderComponent>());
    entity.components.emplace_back(std::make_unique<eol::PlayerComponent>());

    auto animationComponent = std::make_unique<eol::AnimationComponent>();

    eol::Animation idleAnim;
    idleAnim.name = "idle";
    idleAnim.texture = &idleTexture_;
    idleAnim.frameCount = 4;
    idleAnim.frameWidth = 128;
    idleAnim.frameHeight = 128;
    idleAnim.frameDuration = 0.15f;
    idleAnim.loop = true;
    animationComponent->addAnimation("idle", idleAnim);

    eol::Animation walkAnim;
    walkAnim.name = "walk";
    walkAnim.texture = &moveTexture_;
    walkAnim.frameCount = 6;
    walkAnim.frameWidth = 128;
    walkAnim.frameHeight = 128;
    walkAnim.frameDuration = 0.1f;
    walkAnim.loop = true;
    animationComponent->addAnimation("walk", walkAnim);

    animationComponent->setAnimation("idle");
    entity.components.emplace_back(std::move(animationComponent));

    entity.components.emplace_back(std::make_unique<eol::LightComponent>());
    auto emitter = std::make_unique<eol::LightEmitterComponent>();
    emitter->setBeamLength(640.f);
    emitter->setBeamWidth(12.f);
    emitter->setDamage(50.f);
    emitter->setMaxReflections(4);
    entity.components.emplace_back(std::move(emitter));
    entity.components.emplace_back(std::make_unique<eol::UpgradeComponent>());

    return entity;
}

Entity Game::createLightBeaconEntity() {
    Entity entity;
    entity.name = "LightBeacon";
    entity.components.emplace_back(std::make_unique<eol::TransformComponent>(
        sf::Vector2f{640.f, 140.f},
        sf::Vector2f{0.45f, 0.45f},
        0.f));

    auto lightSource = std::make_unique<eol::LightSourceComponent>();
    lightSource->setActive(false);
    lightSource->setMovable(false);
    lightSource->setFuel(0.f);
    entity.components.emplace_back(std::move(lightSource));

    auto puzzle = std::make_unique<eol::PuzzleComponent>();
    puzzle->setRequiredLight(5);
    entity.components.emplace_back(std::move(puzzle));

    auto beaconLight = std::make_unique<eol::LightComponent>();
    beaconLight->setRadius(220.f);
    beaconLight->setBaseIntensity(0.2f);
    entity.components.emplace_back(std::move(beaconLight));

    entity.components.emplace_back(std::make_unique<eol::RenderComponent>());
    return entity;
}

Entity Game::createEnemyEntity() {
    Entity entity;
    entity.name = "Enemy";
    entity.components.emplace_back(std::make_unique<eol::TransformComponent>(
        sf::Vector2f{520.f, 320.f},
        sf::Vector2f{1.f, 1.f},
        0.f));
    entity.components.emplace_back(std::make_unique<eol::EnemyComponent>());

    auto render = std::make_unique<eol::RenderComponent>();
    sf::Sprite& sprite = render->getSprite();
    sprite.setTexture(debugWhiteTexture_);
    sprite.setTextureRect(sf::IntRect(sf::Vector2i{0, 0}, sf::Vector2i{1, 1}));
    sprite.setOrigin(sf::Vector2f{0.5f, 0.5f});
    sprite.setScale(sf::Vector2f{28.f, 36.f});
    render->setTint(sf::Color(255, 110, 110, 240));
    entity.components.emplace_back(std::move(render));

    auto enemyLight = std::make_unique<eol::LightComponent>();
    enemyLight->setBaseIntensity(0.15f);
    enemyLight->setRadius(110.f);
    entity.components.emplace_back(std::move(enemyLight));

    auto melee = std::make_unique<eol::MeleeAttackComponent>();
    melee->setDamage(22.f);
    melee->setRange(60.f);
    melee->setCooldown(1.2f);
    entity.components.emplace_back(std::move(melee));

    return entity;
}

Entity Game::createMirrorEntity(const sf::Vector2f& position,
                                const sf::Vector2f& normal,
                                const sf::Vector2f& size,
                                eol::MirrorComponent::MirrorType type) {
    const auto normalizeVec = [](const sf::Vector2f& vec) {
        const float length = std::sqrt(vec.x * vec.x + vec.y * vec.y);
        if (length <= 0.0001f) {
            return sf::Vector2f{0.f, -1.f};
        }
        return sf::Vector2f{vec.x / length, vec.y / length};
    };

    const sf::Vector2f normalizedNormal = normalizeVec(normal);
    const sf::Vector2f tangent{-normalizedNormal.y, normalizedNormal.x};
    constexpr float radToDeg = 180.f / 3.1415926535f;
    const float rotation = std::atan2(tangent.y, tangent.x) * radToDeg;
    const sf::Vector2f center = position + sf::Vector2f{size.x * 0.5f, size.y * 0.5f};

    Entity entity;
    entity.name = "Mirror";
    entity.components.emplace_back(std::make_unique<eol::TransformComponent>(
        center,
        sf::Vector2f{size.x, size.y},
        rotation));

    auto mirror = std::make_unique<eol::MirrorComponent>();
    mirror->setNormal(normalizedNormal);
    mirror->setSize(size);
    mirror->setType(type);
    entity.components.emplace_back(std::move(mirror));

    auto render = std::make_unique<eol::RenderComponent>();
    sf::Sprite& sprite = render->getSprite();
    sprite.setTexture(debugWhiteTexture_);
    sprite.setTextureRect(sf::IntRect(sf::Vector2i{0, 0}, sf::Vector2i{1, 1}));
    sprite.setOrigin(sf::Vector2f{0.5f, 0.5f});
    render->setTint(sf::Color(160, 210, 255, 220));
    entity.components.emplace_back(std::move(render));

    return entity;
}

Entity Game::createLightSourceNode(const std::string& name,
                                   const sf::Vector2f& position,
                                   bool movable) {
    Entity entity;
    entity.name = name;
    entity.components.emplace_back(std::make_unique<eol::TransformComponent>(
        position,
        sf::Vector2f{0.65f, 0.65f},
        0.f));

    auto source = std::make_unique<eol::LightSourceComponent>();
    source->setMovable(movable);
    source->setActive(true);
    source->setFuel(100.f);
    entity.components.emplace_back(std::move(source));

    auto light = std::make_unique<eol::LightComponent>();
    light->setRadius(180.f);
    light->setBaseIntensity(0.6f);
    entity.components.emplace_back(std::move(light));

    auto render = std::make_unique<eol::RenderComponent>();
    sf::Sprite& sprite = render->getSprite();
    sprite.setTexture(lightNodeTexture_);
    const auto texSize = lightNodeTexture_.getSize();
    sprite.setTextureRect(sf::IntRect(sf::Vector2i{0, 0}, sf::Vector2i(texSize)));
    sprite.setOrigin(sf::Vector2f{static_cast<float>(texSize.x) * 0.5f,
                                  static_cast<float>(texSize.y) * 0.5f});
    render->setTint(movable ? sf::Color(255, 255, 200) : sf::Color(190, 220, 255));
    entity.components.emplace_back(std::move(render));
    return entity;
}

void Game::handleEvents() {
    while (const auto event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                window_.close();
            }
        }
    }
}

void Game::update(float deltaTime) {
    inputSystem_.update(player_, deltaTime, window_);
    animationSystem_.update(entities_, deltaTime);
    combatSystem_.updateMeleeAttacks(entities_, deltaTime);
    lightSystem_.update(entities_, deltaTime, window_);

    // Level progression and win-condition logic will be reinstated when
    // LevelManager support is ready.
}

void Game::render() {
    window_.clear(sf::Color(20, 20, 30));
    renderSystem_.render(window_, entities_);
    lightSystem_.render(window_, entities_);
    window_.display();
}

std::string Game::findResourcePath(const std::string& relativePath) const {
    const std::vector<std::string> possiblePaths = {
        relativePath,
        "../../../" + relativePath,
        "../../../../" + relativePath,
        "D:/code/echoes/echoes-of-Light/" + relativePath
    };

    for (const auto& path : possiblePaths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    return relativePath;
}

