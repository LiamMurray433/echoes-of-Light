#include "Game.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/EnemyAIComponent.h"
#include "components/EnemyComponent.h"
#include "components/HitboxComponent.h"
#include "components/LightComponent.h"
#include "components/LightEmitterComponent.h"
#include "components/LightSourceComponent.h"
#include "components/MeleeAttackComponent.h"
#include "components/MirrorComponent.h"
#include "components/PlayerComponent.h"
#include "components/PuzzleComponent.h"
#include "components/RenderComponent.h"
#include "components/TransformComponent.h"
#include "components/UpgradeComponent.h"
#include "components/LevelManager.h"
#include "components/SpawnerComponent.h"
#include "GameSettings.h"
#include "components/SettingsManager.h"


// =============================================================
//   Helper Image Generators (unchanged from your original)
// =============================================================
namespace {

    sf::Image createSolidImage(unsigned int size, sf::Color color)
    {
        sf::Image img(sf::Vector2u(size, size), color);
        return img;
    }

    sf::Image createCircularFalloffImage(unsigned int size,
        sf::Color centerColor,
        sf::Color edgeColor)
    {
        sf::Image img(sf::Vector2u(size, size), sf::Color::Transparent);

        const float half = static_cast<float>(size) * 0.5f;

        for (unsigned int y = 0; y < size; ++y)
        {
            for (unsigned int x = 0; x < size; ++x)
            {
                float dx = static_cast<float>(x) - half + 0.5f;
                float dy = static_cast<float>(y) - half + 0.5f;
                float dist = std::sqrt(dx * dx + dy * dy);
                float norm = dist / half;

                if (norm <= 1.f)
                {
                    float t = std::clamp(norm, 0.f, 1.f);

                    auto mix = [t](uint8_t a, uint8_t b)
                        {
                            return static_cast<uint8_t>(a * (1.f - t) + b * t);
                        };

                    sf::Color c;
                    c.r = mix(centerColor.r, edgeColor.r);
                    c.g = mix(centerColor.g, edgeColor.g);
                    c.b = mix(centerColor.b, edgeColor.b);
                    c.a = mix(centerColor.a, edgeColor.a);

                    img.setPixel({ x, y }, c);
                }
            }
        }

        return img;
    }

} // namespace

// =============================================================
//   Constructor
// =============================================================
Game::Game()
    : initialized_(false)
    , currentFramerate(60)
    , idleTexture_{}
    , moveTexture_{}
    , debugWhiteTexture_{}
    , lightNodeTexture_{}
    , player_{}
    , enemy_{}
    , entities_{}
    , worldObjects_{}
    , combatSystem_{}
    , enemyAISystem_{}
    , lightSystem_{ combatSystem_ }
{
}

// =============================================================
//   Initialization
// =============================================================
bool Game::initialize()
{
    SettingsManager::get().load();
    window_.setFramerateLimit(SettingsManager::get().framerate);
    setResolution(SettingsManager::get().resolutionIndex);

    std::cout << "=== ECHOES OF LIGHT (Gameplay Initializing) ===\n";

    if (!loadResources())
        return false;

    // Load starting level
    levels_.setCurrentIndex(startLevelIndex_);
    if (!levels_.loadCurrentLevel()) {
        std::cerr << "ERROR: Failed to load starting level\n";
        return false;
    }
    std::cout << "Loaded first level successfully.\n";

    // Calculate tile size for this level
    recalculateTileSize();
    // Set textures for map rendering
    applyWallTextureForCurrentLevel();

    // Initialize dialog system
    if (!dialogSystem_.initialize(gameFont_)) {
        std::cerr << "ERROR: Failed to initialize dialog system\n";
        return false;
    }

    // Check if this is the tutorial level (index 0)
    if (levels_.getCurrentIndex() == 0) {
        // Start interactive tutorial - only show intro + first instruction
        tutorialStep_ = TutorialStep::WaitForMove;
        tutorialActionDetected_ = false;
        dialogSystem_.startDialog({
            {"Narrator", "The kingdom has fallen into darkness..."},
            {"Narrator", "Only one hero remains who can restore the light."},
            {"King", "You must travel through the echoes of time to save us."},
            {"Hero", "I will not fail you, my lord."},
            {"Guide", "Before you begin your journey, let me teach you the ways of light."},
            {"Guide", "Use WASD to move. Try it now!"}
            });
    }
    else {
        // Non-tutorial levels - no interactive tutorial
        tutorialStep_ = TutorialStep::None;
        beaconsPreviouslySolved_ = false;
    }

    // Setting up Enemy spawner system with enemy factory
    spawnerSystem_.setEnemyFactory([this](const sf::Vector2f& position) {
        return createEnemyAtPosition(position);
        });

    createEntities();

    initialized_ = true;
    return true;
}

// =============================================================
//   Resource Loading
// =============================================================
bool Game::loadResources()
{
    std::string idlePath = findResourcePath("resources/sprites/Character_Idle.png");
    std::string movePath = findResourcePath("resources/sprites/Character_Move.png");


    if (!idleTexture_.loadFromFile(idlePath))
    {
        std::cerr << "ERROR: Failed to load idle sprite\n";
        return false;
    }

    if (!moveTexture_.loadFromFile(movePath))
    {
        std::cerr << "ERROR: Failed to load movement sprite\n";
        return false;
    }

    if (!debugWhiteTexture_.loadFromImage(createSolidImage(2, sf::Color::White)))
    {
        std::cerr << "ERROR: Failed to create debug texture\n";
        return false;
    }

    sf::Color center(255, 255, 230, 255);
    sf::Color edge(255, 255, 230, 80);

    if (!lightNodeTexture_.loadFromImage(createCircularFalloffImage(64, center, edge)))
    {
        std::cerr << "ERROR: Failed to create light node texture\n";
        return false;
    }


    // Load wall textures for each era
    std::string wallPastPath = findResourcePath("resources/sprites/PastWall.png");
    if (!wallTexturePast_.loadFromFile(wallPastPath)) {
        std::cerr << "WARNING: Failed to load past wall texture\n";
        if (!wallTexturePast_.loadFromImage(createSolidImage(16, sf::Color(80, 80, 100)))) {
            std::cerr << "ERROR: Failed to create fallback past wall texture\n";
        }
    }

    std::string wallPresentPath = findResourcePath("resources/sprites/PresentWall.png");
    if (!wallTexturePresent_.loadFromFile(wallPresentPath)) {
        std::cerr << "WARNING: Failed to load present wall texture\n";
        if (!wallTexturePresent_.loadFromImage(createSolidImage(16, sf::Color(100, 80, 80)))) {
            std::cerr << "ERROR: Failed to create fallback present wall texture\n";
        }
    }

    std::string wallFuturePath = findResourcePath("resources/sprites/FutureWall.png");
    if (!wallTextureFuture_.loadFromFile(wallFuturePath)) {
        std::cerr << "WARNING: Failed to load future wall texture\n";
        if (!wallTextureFuture_.loadFromImage(createSolidImage(16, sf::Color(80, 100, 100)))) {
            std::cerr << "ERROR: Failed to create fallback future wall texture\n";
        }
    }

    // Load font for dialog system
    std::string fontPath = findResourcePath("resources/fonts/ScienceGothic.ttf");
    if (!gameFont_.openFromFile(fontPath)) {
        std::cerr << "ERROR: Failed to load game font\n";
        return false;
    }

    std::cout << "Textures loaded OK.\n";
    std::cout << "Font loaded OK.\n";
    return true;
}

// =============================================================
//   Create ALL Gameplay Entities (Original Logic Restored)
// =============================================================
void Game::createEntities()
{
    entities_.clear();
    worldObjects_.clear();
    worldObjects_.reserve(64);
    beacons_.clear();

    const Map& map = levels_.getCurrentMap();
    sf::Vector2f playerStartPos = GameSettings::center(); // Default fallback

    auto addWorld = [&](Entity&& e)
        {
            auto ptr = std::make_unique<Entity>();
            *ptr = std::move(e);
            entities_.push_back(ptr.get());
            worldObjects_.push_back(std::move(ptr));
        };

    // Scan through the map and create entities for each tile
    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {
            TileType tile = map.getTile(x, y);
            sf::Vector2f worldPos = tileToWorld(x, y);

            switch (tile) {
            case TileType::WALL:
                addWorld(createWallEntity(worldPos, sf::Vector2f(tileSize_, tileSize_)));
                break;

            case TileType::START:
                playerStartPos = worldPos;
                break;

            case TileType::END:
                // Could create an exit/goal entity here
                // For now, handled by playerReachedExit()
                break;

            case TileType::LIGHT_SOURCE:
                addWorld(createLightSourceNode("Light_" + std::to_string(x) + "_" + std::to_string(y),
                    worldPos, true));
                break;

            case TileType::BEACON_1:
            {
                auto beacon = createLightBeaconEntity(worldPos, 1);
                auto ptr = std::make_unique<Entity>();
                *ptr = std::move(beacon);
                beacons_.push_back(ptr.get());
                entities_.push_back(ptr.get());
                worldObjects_.push_back(std::move(ptr));
                break;
            }
            case TileType::BEACON_2:
            {
                auto beacon = createLightBeaconEntity(worldPos, 2);
                auto ptr = std::make_unique<Entity>();
                *ptr = std::move(beacon);
                beacons_.push_back(ptr.get());
                entities_.push_back(ptr.get());
                worldObjects_.push_back(std::move(ptr));
                break;
            }
            case TileType::BEACON_3:
            {
                auto beacon = createLightBeaconEntity(worldPos, 3);
                auto ptr = std::make_unique<Entity>();
                *ptr = std::move(beacon);
                beacons_.push_back(ptr.get());
                entities_.push_back(ptr.get());
                worldObjects_.push_back(std::move(ptr));
                break;
            }
            case TileType::BEACON_4:
            {
                auto beacon = createLightBeaconEntity(worldPos, 4);
                auto ptr = std::make_unique<Entity>();
                *ptr = std::move(beacon);
                beacons_.push_back(ptr.get());
                entities_.push_back(ptr.get());
                worldObjects_.push_back(std::move(ptr));
                break;
            }


            case TileType::MIRROR:
                addWorld(createMirrorEntity(
                    worldPos,
                    { 1, 1 },
                    GameSettings::relativeSize(0.052f, 0.015f),
                    eol::MirrorComponent::MirrorType::Flat));
                break;

            case TileType::SPAWNER:
                addWorld(createSpawnerEntity(
                    worldPos,
                    5.f,    // Spawn every 5 seconds
                    5       // Max 5 enemies per spawner
                ));
                break;

            case TileType::EMPTY:
            default:
                // Nothing to create
                break;
            }
        }
    }

    // Create player at the START position
    player_ = createPlayerEntity();
    if (auto* transform = player_.getComponent<eol::TransformComponent>()) {
        transform->setPosition(playerStartPos);
    }
    entities_.push_back(&player_);

    // Create light beacon (could place this as a tile from map )
    // Create enemy (you could add an 'X' tile type for enemies)
    enemy_ = createEnemyEntity();
    entities_.push_back(&enemy_);

    std::cout << "Created " << entities_.size() << " entities from map.\n";
}

// =============================================================
//   Entity Creation Functions (ALL ORIGINAL LOGIC RESTORED)
// =============================================================
Entity Game::createPlayerEntity()
{
    Entity e;
    e.name = "Player";

    e.components.emplace_back(std::make_unique<eol::TransformComponent>(
        GameSettings::center(),
        sf::Vector2f{ 0.75f, 0.75f },
        0.f));

    e.components.emplace_back(std::make_unique<eol::RenderComponent>());
    e.components.emplace_back(std::make_unique<eol::PlayerComponent>());

    auto collision = std::make_unique<eol::CollisionComponent>();
    collision->setBoundingBox(GameSettings::relativeSize(0.022f, 0.039f));
    collision->setSolid(true);
    e.components.emplace_back(std::move(collision));

    auto anim = std::make_unique<eol::AnimationComponent>();

    eol::Animation idle;
    idle.name = "idle";
    idle.texture = &idleTexture_;
    idle.frameCount = 4;
    idle.frameWidth = 128;
    idle.frameHeight = 128;
    idle.frameDuration = 0.15f;
    idle.loop = true;
    anim->addAnimation("idle", idle);

    eol::Animation walk;
    walk.name = "walk";
    walk.texture = &moveTexture_;
    walk.frameCount = 6;
    walk.frameWidth = 128;
    walk.frameHeight = 128;
    walk.frameDuration = 0.1f;
    walk.loop = true;
    anim->addAnimation("walk", walk);

    anim->setAnimation("idle");
    e.components.emplace_back(std::move(anim));

    e.components.emplace_back(std::make_unique<eol::LightComponent>());

    auto emitter = std::make_unique<eol::LightEmitterComponent>();
    emitter->setBeamLength(GameSettings::relativeX(0.625f));
    emitter->setBeamWidth(GameSettings::relativeMin(0.015f));
    emitter->setDamage(50.f);
    emitter->setMaxReflections(4);
    e.components.emplace_back(std::move(emitter));

    e.components.emplace_back(std::make_unique<eol::UpgradeComponent>());

    return e;
}

Entity Game::createLightBeaconEntity(const sf::Vector2f& worldPosition, int beaconNumber)
{
    Entity e;
    e.name = "LightBeacon";

    e.components.emplace_back(std::make_unique<eol::TransformComponent>(
        worldPosition,
        sf::Vector2f{ 0.65f, 0.65f },
        0.f));

    auto src = std::make_unique<eol::LightSourceComponent>();
    src->setMovable(true);
    src->setActive(false);
    src->setFuel(0.f);
    e.components.emplace_back(std::move(src));

    auto puzzle = std::make_unique<eol::PuzzleComponent>();
    puzzle->setRequiredLight(1);
    puzzle->setSolved(false);
    puzzle->setLightRequirement(eol::PuzzleComponent::LightRequirement::Any);
    puzzle->setRequiredUniqueSources(beaconNumber);
    e.components.emplace_back(std::move(puzzle));

    auto hitbox = std::make_unique<eol::HitboxComponent>();
    hitbox->setSize(GameSettings::relativeSize(0.05f, 0.05f));
    e.components.emplace_back(std::move(hitbox));

    auto light = std::make_unique<eol::LightComponent>();
    light->setRadius(GameSettings::relativeMin(0.324f));
    light->setBaseIntensity(0.15f);
    e.components.emplace_back(std::move(light));

    auto emitter = std::make_unique<eol::LightEmitterComponent>();
    emitter->setDirection(sf::Vector2f{0.f, -1.f});
    emitter->setBeamLength(GameSettings::relativeY(0.95f));
    emitter->setBeamWidth(GameSettings::relativeMin(0.012f));
    emitter->setDamage(55.f);
    emitter->setBeamDuration(0.18f);
    emitter->setCooldown(0.1f);
    emitter->setMaxReflections(6);
    emitter->setBeamColor(sf::Color(255, 242, 205, 255));
    emitter->setContinuousFire(true);
    emitter->setTriggerHeld(false);
    e.components.emplace_back(std::move(emitter));

    e.components.emplace_back(std::make_unique<eol::RenderComponent>());
    return e;
}

Entity Game::createEnemyEntity()
{
    Entity e;
    e.name = "Enemy";

    e.components.emplace_back(std::make_unique<eol::TransformComponent>(
        GameSettings::relativePos(0.65f, 0.533f),
        sf::Vector2f{ 1.5f, 1.5f },
        0.f));

    auto hitbox = std::make_unique<eol::HitboxComponent>();
    hitbox->setSize(GameSettings::relativeSize(0.06f, 0.1f));
    e.components.emplace_back(std::move(hitbox));

    e.components.emplace_back(std::make_unique<eol::EnemyComponent>());

    auto render = std::make_unique<eol::RenderComponent>();
    auto& sprite = render->getSprite();
    sprite.setTexture(debugWhiteTexture_);
    sprite.setTextureRect({ {0,0}, {1,1} });
    sprite.setOrigin({ 0.5f, 0.5f });
    sprite.setScale(GameSettings::relativeSize(0.022f, 0.05f));
    render->setTint(sf::Color(255, 110, 110, 240));
    e.components.emplace_back(std::move(render));

    auto melee = std::make_unique<eol::MeleeAttackComponent>();
    melee->setDamage(22.f);
    melee->setRange(GameSettings::relativeMin(0.083f));
    melee->setCooldown(1.2f);
    e.components.emplace_back(std::move(melee));

    auto ai = std::make_unique<eol::EnemyAIComponent>();
    const auto basePos = GameSettings::relativePos(0.65f, 0.533f);
    ai->setPatrolPoints({ basePos, basePos + sf::Vector2f{-GameSettings::relativeX(0.175f), 0.f} });
    ai->setDetectionRange(GameSettings::relativeMin(0.37f));
    ai->setAttackRange(GameSettings::relativeMin(0.014f));
    ai->setMoveSpeed(GameSettings::relativeMin(0.046f));
    e.components.emplace_back(std::move(ai));

    return e;
}

Entity Game::createMirrorEntity(const sf::Vector2f& position,
    const sf::Vector2f& normal,
    const sf::Vector2f& size,
    eol::MirrorComponent::MirrorType type)
{
    const auto normalize = [](sf::Vector2f v)
        {
            float len = std::sqrt(v.x * v.x + v.y * v.y);
            return (len > 0.0001f) ? sf::Vector2f(v.x / len, v.y / len)
                : sf::Vector2f(0.f, -1.f);
        };

    sf::Vector2f n = normalize(normal);
    sf::Vector2f tangent(-n.y, n.x);
    float rotation = std::atan2(tangent.y, tangent.x) * 180.f / 3.1415926f;

    sf::Vector2f center = position + sf::Vector2f(size.x * 0.5f, size.y * 0.5f);

    Entity e;
    e.name = "Mirror";

    e.components.emplace_back(std::make_unique<eol::TransformComponent>(
        center, sf::Vector2f(size.x, size.y), rotation));

    auto comp = std::make_unique<eol::MirrorComponent>();
    comp->setNormal(n);
    comp->setSize(size);
    comp->setType(type);
    e.components.emplace_back(std::move(comp));

    auto render = std::make_unique<eol::RenderComponent>();
    auto& sprite = render->getSprite();
    sprite.setTexture(debugWhiteTexture_);
    sprite.setTextureRect({ {0,0}, {1,1} });
    sprite.setOrigin({ 0.5f, 0.5f });
    render->setTint(sf::Color(160, 210, 255, 220));
    e.components.emplace_back(std::move(render));

    return e;
}

Entity Game::createLightSourceNode(
    const std::string& name,
    const sf::Vector2f& position,
    bool movable)
{
    Entity e;
    e.name = name;

    e.components.emplace_back(std::make_unique<eol::TransformComponent>(
        position, sf::Vector2f{ 1.f, 1.f }, 0.f));

    auto src = std::make_unique<eol::LightSourceComponent>();
    src->setMovable(movable);
    src->setActive(true);
    src->setFuel(100.f);
    e.components.emplace_back(std::move(src));

    auto light = std::make_unique<eol::LightComponent>();
    light->setRadius(GameSettings::relativeMin(0.259f));
    light->setBaseIntensity(0.6f);
    e.components.emplace_back(std::move(light));

    auto render = std::make_unique<eol::RenderComponent>();
    auto& sprite = render->getSprite();
    sprite.setTexture(lightNodeTexture_);
    sf::Vector2u tex = lightNodeTexture_.getSize();
    sprite.setTextureRect({ {0,0}, sf::Vector2i(tex) });
    sprite.setOrigin({ tex.x * 0.5f, tex.y * 0.5f });
    render->setTint(movable ? sf::Color(255, 255, 200)
        : sf::Color(190, 220, 255));
    e.components.emplace_back(std::move(render));

    return e;
}

Entity Game::createWallEntity(const sf::Vector2f& pos, const sf::Vector2f& size)
{
    Entity e;
    e.name = "Wall";

    e.components.emplace_back(std::make_unique<eol::TransformComponent>(
        pos,
        sf::Vector2f(1.f, 1.f),  // Scale = 1, we set size directly
        0.f));

    auto coll = std::make_unique<eol::CollisionComponent>();
    coll->setBoundingBox(size);
    coll->setSolid(true);
    e.components.emplace_back(std::move(coll));

    auto render = std::make_unique<eol::RenderComponent>();
    auto& sprite = render->getSprite();
    sprite.setTexture(debugWhiteTexture_);
    sprite.setTextureRect({ {0, 0}, {2, 2} });
    sprite.setOrigin({ 1.f, 1.f });
    sprite.setScale(sf::Vector2f(size.x * 0.5f, size.y * 0.5f));
    render->setTint(sf::Color(80, 80, 100, 220));  // Match map color
    e.components.emplace_back(std::move(render));

    return e;
}

Entity Game::createSpawnerEntity(const sf::Vector2f& position, float interval, int maxEnemies)
{
    Entity e;
    e.name = "Spawner";

    // Position only - (invisible)
    e.components.emplace_back(std::make_unique<eol::TransformComponent>(
        position,
        sf::Vector2f{ 1.f, 1.f },
        0.f));

    // Spawner settings
    auto spawner = std::make_unique<eol::SpawnerComponent>();
    spawner->setSpawnInterval(interval);
    spawner->setMaxEnemies(maxEnemies);
    e.components.emplace_back(std::move(spawner));

    return e;
}

Entity Game::createEnemyAtPosition(const sf::Vector2f& position)
{
    // Create a standard enemy using existing function
    Entity e = createEnemyEntity();

    // Override the position
    if (auto* transform = e.getComponent<eol::TransformComponent>()) {
        transform->setPosition(position);
    }

    // Update patrol points to be around the spawn position
    if (auto* ai = e.getComponent<eol::EnemyAIComponent>()) {
        ai->setPatrolPoints({
            position,
            position + sf::Vector2f{-GameSettings::relativeX(0.1f), 0.f},
            position + sf::Vector2f{GameSettings::relativeX(0.1f), 0.f}
            });
    }

    return e;
}

// =============================================================
//   UPDATE (Scene system calls this)
// =============================================================
void Game::update(float dt, sf::RenderWindow& window)
{
    handleEvents();

    // Update dialog system first
    dialogSystem_.update(dt);

    // Update interactive tutorial (checks for player actions)
    if (tutorialStep_ != TutorialStep::None && tutorialStep_ != TutorialStep::Complete) {
        updateTutorial();
    }

    // Only update gameplay if dialog is not active (pauses game during dialog)
    if (!dialogSystem_.isActive()) {
        inputSystem_.updateWithCollision(player_, dt, window, entities_);
        animationSystem_.update(entities_, dt);
        enemyAISystem_.update(entities_, dt, player_);
        combatSystem_.updateMeleeAttacks(entities_, dt);

        // Update spawners and add new enemies
        std::vector<Entity> newEnemies = spawnerSystem_.update(entities_, dt);
        for (auto& enemy : newEnemies) {
            auto ptr = std::make_unique<Entity>(std::move(enemy));
            entities_.push_back(ptr.get());
            worldObjects_.push_back(std::move(ptr));
        }


        // Check if a beacon was just solved and show hint for next one
         // Skip during tutorial - tutorial system handles it
        if (tutorialStep_ == TutorialStep::None && !isBeaconPuzzleSolved()) {
            // Count how many beacons are solved and find the highest solved requirement
            int highestSolvedRequirement = 0;
            for (Entity* beacon : beacons_) {
                if (!beacon) continue;
                auto* puzzle = beacon->getComponent<eol::PuzzleComponent>();
                if (puzzle && puzzle->isSolved()) {
                    int req = static_cast<int>(puzzle->getRequiredUniqueSources());
                    if (req > highestSolvedRequirement) {
                        highestSolvedRequirement = req;
                    }
                }
            }

            // Show hint if we solved a new beacon
            if (highestSolvedRequirement > lastBeaconHintShown_) {
                lastBeaconHintShown_ = highestSolvedRequirement;

                // Find what the next beacon requires
                int nextRequirement = 0;
                for (Entity* beacon : beacons_) {
                    if (!beacon) continue;
                    auto* puzzle = beacon->getComponent<eol::PuzzleComponent>();
                    if (puzzle && !puzzle->isSolved()) {
                        int req = static_cast<int>(puzzle->getRequiredUniqueSources());
                        if (nextRequirement == 0 || req < nextRequirement) {
                            nextRequirement = req;
                        }
                    }
                }

                // Show appropriate hint based on next beacon's requirement
                if (nextRequirement == 2) {
                    dialogSystem_.startDialog({
                        {"Guide", "The beacon shines! Well done."},
                        {"Guide", "The NEXT BEACON requires light from TWO different sources."},
                        {"Guide", "Use your light AND a beacon's beam together!"}
                        });
                }
                else if (nextRequirement == 3) {
                    dialogSystem_.startDialog({
                        {"Guide", "Excellent! Another beacon activated."},
                        {"Guide", "The NEXT BEACON requires light from THREE different sources!"},
                        {"Guide", "Combine your light with multiple beacon beams!"}
                        });
                }
                else if (nextRequirement > 3) {
                    dialogSystem_.startDialog({
                        {"Guide", "Well done! Keep going."},
                        {"Guide", "The NEXT BEACON requires light from " + std::to_string(nextRequirement) + " different sources!"}
                        });
                }
                else if (nextRequirement == 4) {
                    dialogSystem_.startDialog({
                        {"Guide", "Brilliant! The beacon awakens."},
                        {"Guide", "The FINAL BEACON requires light from FOUR different sources!"},
                        {"Guide", "Combine your light with ALL activated beacon beams!"}
                        });
                }
                else {
                    dialogSystem_.startDialog({
                        {"Guide", "The beacon shines! Find and activate the next one."}
                        });
                }

                lightSystem_.update(entities_, dt, window);
                return;
            }
        }

        // Check if all beacons just got solved (show message once)
        // Skip this message during tutorial - the tutorial system handles it
        if (tutorialStep_ == TutorialStep::None && allBeaconsJustSolved()) {
            dialogSystem_.startDialog({
                {"Guide", "The beacons shine bright! The path forward is open."},
                {"Guide", "Make your way to the EXIT."}
                });
            lightSystem_.update(entities_, dt, window);
            return;
        }

        // Check if player reached the exit and required puzzle(s) are solved
        if (!gameComplete_ && isBeaconPuzzleSolved() && playerReachedExit()) {
            int previousLevel = levels_.getCurrentIndex();
            levels_.nextLevel();

            if (levels_.isLevelComplete()) {
                gameComplete_ = true;
                // All levels complete - show victory message
                dialogSystem_.startDialog({
                    {"Narrator", "Congratulations! You have restored the light to all eras!"},
                    {"King", "The kingdom is saved. You are a true hero!"}
                    });
            }
            else {
                // Load next level
                recalculateTileSize();
                applyWallTextureForCurrentLevel();
                createEntities();
                beaconsPreviouslySolved_ = false; // Reset for new level
                tutorialStep_ = TutorialStep::None;
                lastBeaconHintShown_ = 0;  // Reset hints for new level

                // Show era-specific transition dialog
                int newLevel = levels_.getCurrentIndex();

                if (previousLevel == 0) {
                    // Completed tutorial, entering Past era
                    dialogSystem_.startDialog({
                        {"Guide", "Well done! You have mastered the basics."},
                        {"Narrator", "Your journey through time begins now..."},
                        {"Narrator", "The PAST awaits. Ancient ruins hold forgotten secrets."},
                        {"Guide", "Beware - shadows now roam these halls."}
                        });
                }
                else if (newLevel == 2) {
                    // Entering Present era
                    dialogSystem_.startDialog({
                        {"Narrator", "The echoes of the past fade behind you..."},
                        {"Narrator", "You step into the PRESENT. The world you once knew."},
                        {"Narrator", "But darkness has taken hold here too."},
                        {"Guide", "The puzzles grow more complex. Stay vigilant."}
                        });
                }
                else if (newLevel == 3) {
                    // Entering Future era
                    dialogSystem_.startDialog({
                        {"Narrator", "Time bends around you as you leap forward..."},
                        {"Narrator", "The FUTURE stretches before you, cold and uncertain."},
                        {"Narrator", "This is where the darkness originated."},
                        {"Guide", "Your final trial awaits. The fate of all eras rests on you."}
                        });
                }

                else {
                    // Generic transition within same era
                    dialogSystem_.startDialog({
                        {"Narrator", "You have found the path forward..."},
                        {"Narrator", "A new challenge awaits."}
                        });
                }
            }
        }
    }
        // Light system updates regardless (for visual effects)
        lightSystem_.update(entities_, dt, window);
    }


// =============================================================
//   RENDER (Scene system calls this)
// =============================================================
void Game::render(sf::RenderWindow& window)
{

    // Draw the map first (background layer)
    const Map& map = levels_.getCurrentMap();
    if (map.getWidth() > 0 && map.getHeight() > 0) {
        map.draw(window, tileSize_, mapOffset_);
    }

    renderSystem_.render(window, entities_);
    lightSystem_.render(window, entities_);

    // Render dialog on top of everything
    dialogSystem_.render(window);
}

// =============================================================
//   Utilities
// =============================================================
void Game::setFramerateLimit(unsigned int limit)
{
    currentFramerate = limit;
}

unsigned int Game::getFramerateLimit() const
{
    return currentFramerate;
}

std::string Game::findResourcePath(const std::string& relative) const
{
    const std::vector<std::string> paths = {
        relative,
        "../../../" + relative,
        "../../../../" + relative,
        "D:/code/echoes/echoes-of-Light/" + relative
    };

    for (auto& p : paths)
        if (std::filesystem::exists(p))
            return p;

    return relative;
}


bool Game::playerReachedExit() {
    // Don't check if game is already complete
    if (levels_.isLevelComplete()) {
        return false;
    }

    // Get player's world position
    auto* t = player_.getComponent<eol::TransformComponent>();
    if (!t) return false;

    sf::Vector2f ppos = t->getPosition();

    // Get exit tile world position
    LevelObjects objs = levels_.scanObjects();
    if (objs.exitTile.x < 0) return false;

    // Convert exit tile to world coordinates (center of tile)
    sf::Vector2f exitWorld = tileToWorld(objs.exitTile.x, objs.exitTile.y);

    // Check distance
    float dx = ppos.x - exitWorld.x;
    float dy = ppos.y - exitWorld.y;
    float dist2 = dx * dx + dy * dy;

    // Within half a tile
    float threshold = tileSize_ * 0.5f;
    return dist2 < (threshold * threshold);
}

bool Game::isBeaconPuzzleSolved() {
    if (beacons_.empty()) {
        return true;
    }

    for (Entity* beacon : beacons_) {
        if (!beacon) continue;
        if (auto* puzzle = beacon->getComponent<eol::PuzzleComponent>()) {
            if (!puzzle->isSolved()) {
                return false;
            }
        }
    }

    return true;
}

sf::Vector2f Game::tileToWorld(int tileX, int tileY) const {
    // Returns the center of the tile in world coordinates
    return sf::Vector2f(
        mapOffset_.x + (tileX + 0.5f) * tileSize_,
        mapOffset_.y + (tileY + 0.5f) * tileSize_
    );
}

void Game::recalculateTileSize()
{
    const Map& map = levels_.getCurrentMap();
    if (map.getWidth() > 0 && map.getHeight() > 0) {
        float tileSizeX = GameSettings::width() / static_cast<float>(map.getWidth());
        float tileSizeY = GameSettings::height() / static_cast<float>(map.getHeight());
        tileSize_ = std::min(tileSizeX, tileSizeY);

        // Calculate offset to center the map
        float mapPixelWidth = map.getWidth() * tileSize_;
        float mapPixelHeight = map.getHeight() * tileSize_;
        mapOffset_.x = (GameSettings::width() - mapPixelWidth) / 2.f;
        mapOffset_.y = (GameSettings::height() - mapPixelHeight) / 2.f;
    }
}

void Game::applyWallTextureForCurrentLevel() {
    int levelIndex = levels_.getCurrentIndex();

    // Level 0: Tutorial, Level 1: Past, Level 2: Present, Level 3: Future
    if (levelIndex <= 1) {
        levels_.getCurrentMapMutable().setWallTexture(wallTexturePast_);
    }
    else if (levelIndex == 2) {
        levels_.getCurrentMapMutable().setWallTexture(wallTexturePresent_);
    }
    else {
        levels_.getCurrentMapMutable().setWallTexture(wallTextureFuture_);
    }
}
void Game::handleEvents()
{
    sf::Event event;
    while (window_.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
            window_.close();

        if (event.type == sf::Event::KeyPressed)
        {
            switch (event.key.code)
            {
            // Resolution save keys
            case sf::Keyboard::F1:
                setResolution(0);
                SettingsManager::get().resolutionIndex = 0;
                SettingsManager::get().save();
                break;

            case sf::Keyboard::F2:
                setResolution(1);
                SettingsManager::get().resolutionIndex = 1;
                SettingsManager::get().save();
                break;

            case sf::Keyboard::F3:
                setResolution(2);
                SettingsManager::get().resolutionIndex = 2;
                SettingsManager::get().save();
                break;

            //Framerate save keys
            case sf::Keyboard::Num1:  // 60 FPS
                window_.setFramerateLimit(60);
                SettingsManager::get().framerate = 60;
                SettingsManager::get().save();
                break;

            case sf::Keyboard::Num2:  // 120 FPS
                window_.setFramerateLimit(120);
                SettingsManager::get().framerate = 120;
                SettingsManager::get().save();
                break;

            case sf::Keyboard::Num3:  // unlimited
                window_.setFramerateLimit(0);
                SettingsManager::get().framerate = 0;
                SettingsManager::get().save();
                break;
            }
        }
    }
}



bool Game::allBeaconsJustSolved() {
    // If no beacons, nothing to check
    if (beacons_.empty()) {
        return false;
    }

    // If we already showed the message, don't show again
    if (beaconsPreviouslySolved_) {
        return false;
    }

    // Check if all beacons are now solved
    bool allSolved = isBeaconPuzzleSolved();

    // If all solved and we haven't shown message yet
    if (allSolved) {
        beaconsPreviouslySolved_ = true;
        return true;
    }

    return false;
}

void Game::updateTutorial() {
    // Don't check while dialog is showing - let player read first
    if (dialogSystem_.isActive()) {
        return;
    }

    bool actionPerformed = false;

    switch (tutorialStep_) {
    case TutorialStep::WaitForMove:
        // Check if player pressed any movement key
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            tutorialActionDetected_ = true;
        }
        // Only advance after key is released (confirms they actually tried it)
        if (tutorialActionDetected_ &&
            !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) &&
            !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) &&
            !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) &&
            !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            actionPerformed = true;
        }
        break;

    case TutorialStep::WaitForShoot:
        // Check if player shot light
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
            tutorialActionDetected_ = true;
        }
        if (tutorialActionDetected_ &&
            !sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) &&
            !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
            actionPerformed = true;
        }
        break;

    case TutorialStep::WaitForBeacon1:
        // Check if first beacon (requires 1 source) is activated
        for (Entity* beacon : beacons_) {
            if (!beacon) continue;
            auto* puzzle = beacon->getComponent<eol::PuzzleComponent>();
            if (puzzle && puzzle->getRequiredUniqueSources() == 1 && puzzle->isSolved()) {
                actionPerformed = true;
                break;
            }
        }
        break;

    case TutorialStep::WaitForMirrorPickup:
        // Check if player picked up a MIRROR specifically
    {
        auto* playerComp = player_.getComponent<eol::PlayerComponent>();
        if (playerComp && playerComp->isCarrying()) {
            Entity* carried = playerComp->getCarriedEntity();
            if (carried && carried->getComponent<eol::MirrorComponent>()) {
                actionPerformed = true;
            }
        }
    }
    break;

    case TutorialStep::WaitForMirrorRotate:
        // Check if R key was pressed while carrying mirror
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
            auto* playerComp = player_.getComponent<eol::PlayerComponent>();
            if (playerComp && playerComp->isCarrying()) {
                Entity* carried = playerComp->getCarriedEntity();
                if (carried && carried->getComponent<eol::MirrorComponent>()) {
                    tutorialActionDetected_ = true;
                }
            }
        }
        if (tutorialActionDetected_ && !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
            actionPerformed = true;
        }
        break;

    case TutorialStep::WaitForMirrorDrop:
        // Check if player dropped the mirror (no longer carrying anything)
    {
        auto* playerComp = player_.getComponent<eol::PlayerComponent>();
        if (playerComp && !playerComp->isCarrying()) {
            actionPerformed = true;
        }
    }
    break;

    case TutorialStep::WaitForBeaconPickup:
        // Check if player picked up a BEACON specifically (has LightSourceComponent)
    {
        auto* playerComp = player_.getComponent<eol::PlayerComponent>();
        if (playerComp && playerComp->isCarrying()) {
            Entity* carried = playerComp->getCarriedEntity();
            if (carried && carried->getComponent<eol::LightSourceComponent>()) {
                actionPerformed = true;
            }
        }
    }
    break;

    case TutorialStep::WaitForBeaconRotate:
        // Check if R key was pressed while carrying beacon
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
            auto* playerComp = player_.getComponent<eol::PlayerComponent>();
            if (playerComp && playerComp->isCarrying()) {
                Entity* carried = playerComp->getCarriedEntity();
                if (carried && carried->getComponent<eol::LightSourceComponent>()) {
                    tutorialActionDetected_ = true;
                }
            }
        }
        if (tutorialActionDetected_ && !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
            actionPerformed = true;
        }
        break;

    case TutorialStep::WaitForBeaconDrop:
        // Check if player dropped the beacon
    {
        auto* playerComp = player_.getComponent<eol::PlayerComponent>();
        if (playerComp && !playerComp->isCarrying()) {
            actionPerformed = true;
        }
    }
    break;

    case TutorialStep::WaitForBeacon2:
        // Check if all beacons are solved
        if (isBeaconPuzzleSolved()) {
            actionPerformed = true;
        }
        break;

    default:
        break;
    }

    if (actionPerformed) {
        advanceTutorial();
    }
}

void Game::advanceTutorial() {
    // Reset action flag for next step
    tutorialActionDetected_ = false;

    switch (tutorialStep_) {
    case TutorialStep::WaitForMove:
        tutorialStep_ = TutorialStep::WaitForShoot;
        dialogSystem_.startDialog({
            {"Guide", "Excellent! You move with grace."},
            {"Guide", "Now point your MOUSE and LEFT CLICK or press SPACE to emit light."},
            {"Guide", "Try it now!"}
            });
        break;

    case TutorialStep::WaitForShoot:
        tutorialStep_ = TutorialStep::WaitForBeacon1;
        dialogSystem_.startDialog({
            {"Guide", "Your light shines bright!"},
            {"Guide", "See that beacon ahead? It needs your light to awaken."},
            {"Guide", "Aim your beam at the FIRST BEACON and hold until it activates!"}
            });
        break;

    case TutorialStep::WaitForBeacon1:
        tutorialStep_ = TutorialStep::WaitForMirrorPickup;
        dialogSystem_.startDialog({
            {"Guide", "The first beacon awakens! See how it emits light upward?"},
            {"Guide", "Now find the MIRROR nearby. Walk close and press E to pick it up."}
            });
        break;

    case TutorialStep::WaitForMirrorPickup:
        tutorialStep_ = TutorialStep::WaitForMirrorRotate;
        dialogSystem_.startDialog({
            {"Guide", "You've grabbed the mirror! It follows you now."},
            {"Guide", "Press R to ROTATE the mirror 45 degrees. Try it!"}
            });
        break;

    case TutorialStep::WaitForMirrorRotate:
        tutorialStep_ = TutorialStep::WaitForMirrorDrop;
        dialogSystem_.startDialog({
            {"Guide", "Perfect! Mirrors redirect light beams."},
            {"Guide", "Press E to DROP the mirror where you want it."}
            });
        break;

    case TutorialStep::WaitForMirrorDrop:
        tutorialStep_ = TutorialStep::WaitForBeaconPickup;
        dialogSystem_.startDialog({
            {"Guide", "Good! The mirror is placed."},
            {"Guide", "Did you know? Beacons can ALSO be picked up and rotated!"},
            {"Guide", "Walk to the BEACON and press E to pick it up."}
            });
        break;

    case TutorialStep::WaitForBeaconPickup:
        tutorialStep_ = TutorialStep::WaitForBeaconRotate;
        dialogSystem_.startDialog({
            {"Guide", "You're carrying the beacon! Its light follows you now."},
            {"Guide", "Press R to ROTATE the beacon's light direction. Try it!"}
            });
        break;

    case TutorialStep::WaitForBeaconRotate:
        tutorialStep_ = TutorialStep::WaitForBeaconDrop;
        dialogSystem_.startDialog({
            {"Guide", "Excellent! You can aim the beacon's light wherever you need it."},
            {"Guide", "Press E to DROP the beacon."}
            });
        break;

    case TutorialStep::WaitForBeaconDrop:
        tutorialStep_ = TutorialStep::WaitForBeacon2;
        dialogSystem_.startDialog({
            {"Guide", "Now for the final challenge!"},
            {"Guide", "The SECOND BEACON needs light from TWO sources to activate."},
            {"Guide", "Use the mirror to redirect the first beacon's light to beacon 2."},
            {"Guide", "Then add YOUR light to the second beacon as well!"}
            });
        break;

    case TutorialStep::WaitForBeacon2:
        tutorialStep_ = TutorialStep::Complete;
        dialogSystem_.startDialog({
            {"Guide", "BRILLIANT! All beacons shine!"},
            {"Guide", "The EXIT is now open - find the red tile to proceed."},
            {"Guide", "You have mastered the basics. Your real journey begins now!"}
            });
        break;

    default:
        break;
    }
}