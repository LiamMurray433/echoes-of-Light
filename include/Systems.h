#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "components/Component.h"
#include "components/LightEmitterComponent.h"

// Simple Entity structure (same as before)
struct Entity {
    std::string name;
    std::vector<eol::ComponentPtr> components;

    template<typename T>
    T* getComponent() {
        for (auto& comp : components) {
            if (T* result = dynamic_cast<T*>(comp.get())) {
                return result;
            }
        }
        return nullptr;
    }

    template<typename T>
    bool hasComponent() {
        return getComponent<T>() != nullptr;
    }
};


// INPUT SYSTEM - Handles player keyboard input
class InputSystem {
public:
    void update(Entity& player, float deltaTime, const sf::RenderWindow& window);

private:
    sf::Vector2f getMovementInput() const;
    void updatePlayerEmitter(Entity& player, const sf::RenderWindow& window);
};

// ANIMATION SYSTEM - Updates all entity animations
class AnimationSystem {
public:
    void update(std::vector<Entity*>& entities, float deltaTime);
};

// RENDER SYSTEM - Draws all entities to the screen
class RenderSystem {
public:
    void render(sf::RenderWindow& window, std::vector<Entity*>& entities);

private:
    void updateSpriteFromComponents(sf::Sprite& sprite, Entity& entity);
};

// COMBAT SYSTEM - Handles HP, resistances and hit reactions
class CombatSystem {
public:
    void applyBeamHit(Entity& attacker, Entity& target, float intensity, const sf::Vector2f& hitPoint);
    void applyMeleeHit(Entity& attacker, Entity& target, float damage);
    void updateMeleeAttacks(std::vector<Entity*>& entities, float deltaTime);

private:
    bool applyEnemyHit(Entity& target, float intensity);
    void applyPlayerDamage(Entity& attacker, Entity& target, float damage);
    Entity* findPlayer(const std::vector<Entity*>& entities) const;
};


// LIGHT SYSTEM - Handles emission, reflections and beam combat
class LightSystem {
public:
    explicit LightSystem(CombatSystem& combatSystem);

    void update(std::vector<Entity*>& entities, float deltaTime, const sf::RenderWindow& window);
    void render(sf::RenderTarget& target, std::vector<Entity*>& entities);

    void setAmbientLight(float ambient) noexcept;
    float getAmbientLight() const noexcept;

    void setDebugOverlayEnabled(bool enabled) noexcept;
    bool isDebugOverlayEnabled() const noexcept;

private:
    struct BeamSegment {
        sf::Vector2f start;
        sf::Vector2f end;
        sf::Color color;
        float width;
        float ttl;
        float lifetime;
        float intensity;
    };

    void updateEmitters(std::vector<Entity*>& entities, float deltaTime, const sf::RenderWindow& window);
    void updateLightFields(std::vector<Entity*>& entities, float deltaTime);
    void refreshBeamTimers(float deltaTime);
    void emitBeam(Entity& owner,
                  eol::LightEmitterComponent& emitter,
                  std::vector<Entity*>& entities,
                  const sf::Vector2f& origin,
                  const sf::Vector2f& direction);
    void castBeam(Entity& owner,
                  std::vector<Entity*>& entities,
                  const sf::Vector2f& origin,
                  const sf::Vector2f& direction,
                  float range,
                  float width,
                  sf::Color color,
                  float intensity,
                  float ttl,
                  std::uint32_t reflectionsLeft);
    bool rayIntersectsRect(const sf::Vector2f& origin,
                           const sf::Vector2f& direction,
                           float maxDistance,
                           const sf::FloatRect& bounds,
                           float& outDistance,
                           sf::Vector2f& outNormal) const;
    bool rayIntersectsMirror(const sf::Vector2f& origin,
                             const sf::Vector2f& direction,
                             float maxDistance,
                             Entity& mirrorEntity,
                             float& outDistance,
                             sf::Vector2f& outNormal) const;
    std::optional<sf::FloatRect> computeBounds(Entity& entity) const;
    std::optional<sf::FloatRect> computeMirrorBounds(Entity& entity) const;
    void handleBeamImpact(Entity& owner, Entity& target, float intensity, const sf::Vector2f& hitPoint);
    void applyPuzzleLight(Entity& entity, float intensity);
    sf::Vector2f reflect(const sf::Vector2f& direction, const sf::Vector2f& normal) const;
    void ensureOverlaySize(const sf::RenderTarget& target);
    void drawBeams(sf::RenderTarget& target) const;
    void drawOverlay(sf::RenderTarget& target) const;
    void drawDebugData(sf::RenderTarget& target) const;
    void drawLightGlows(sf::RenderTarget& target, std::vector<Entity*>& entities);

private:
    std::vector<BeamSegment> m_beamSegments;
    sf::RectangleShape m_darknessOverlay;
    bool m_debugOverlay;
    float m_ambientLight;
    std::vector<sf::FloatRect> m_debugMirrorBounds;
    std::vector<sf::Vector2f> m_debugHitPoints;
    CombatSystem& m_combat;
};


