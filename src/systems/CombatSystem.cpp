#include "Systems.h"

#include "components/EnemyComponent.h"
#include "components/LightComponent.h"
#include "components/MeleeAttackComponent.h"
#include "components/PlayerComponent.h"
#include "components/RenderComponent.h"
#include "components/TransformComponent.h"

#include <algorithm>

namespace {

float clampf(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

float distanceSquared(const sf::Vector2f& a, const sf::Vector2f& b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

} // namespace

void CombatSystem::applyBeamHit(Entity& attacker,
                                Entity& target,
                                float intensity,
                                const sf::Vector2f& /*hitPoint*/) {
    if (applyEnemyHit(target, intensity)) {
        return;
    }

    constexpr float baseDamage = 8.f;
    float damage = baseDamage + intensity * 0.15f;
    if (attacker.getComponent<eol::EnemyComponent>()) {
        damage *= 1.25f;
    }

    applyPlayerDamage(attacker, target, damage);
}

void CombatSystem::applyMeleeHit(Entity& attacker, Entity& target, float damage) {
    applyPlayerDamage(attacker, target, damage);
}

void CombatSystem::updateMeleeAttacks(std::vector<Entity*>& entities, float deltaTime) {
    Entity* player = findPlayer(entities);
    if (!player) {
        return;
    }

    auto* playerTransform = player->getComponent<eol::TransformComponent>();
    if (!playerTransform) {
        return;
    }

    const sf::Vector2f playerPos = playerTransform->getPosition();

    for (Entity* entity : entities) {
        if (!entity) continue;

        auto* melee = entity->getComponent<eol::MeleeAttackComponent>();
        auto* transform = entity->getComponent<eol::TransformComponent>();
        if (!melee || !transform || !melee->isEnabled()) {
            continue;
        }

        melee->advanceCooldown(deltaTime);

        const float range = melee->getRange();
        if (distanceSquared(transform->getPosition(), playerPos) <= range * range) {
            if (melee->canAttack()) {
                applyMeleeHit(*entity, *player, melee->getDamage());
                melee->resetCooldown();
            }
        }
    }
}

bool CombatSystem::applyEnemyHit(Entity& target, float intensity) {
    auto* enemy = target.getComponent<eol::EnemyComponent>();
    if (!enemy) {
        return false;
    }

    const float resistanceLoss = clampf(intensity * 0.04f, 0.f, 5.f);
    enemy->setResistance(std::max(0.f, enemy->getResistance() - resistanceLoss));

    const float mitigation = clampf(enemy->getResistance(), 0.f, 1.f);
    const float damage = intensity * (0.35f + (1.f - mitigation) * 0.5f);
    enemy->applyDamage(damage);

    if (!enemy->isAlive()) {
        enemy->setBlocksLight(false);
        if (auto* render = target.getComponent<eol::RenderComponent>()) {
            render->setTint(sf::Color(30, 30, 30, 160));
        }
        if (auto* light = target.getComponent<eol::LightComponent>()) {
            light->setIntensity(light->getBaseIntensity());
        }
        return true;
    }

    if (auto* render = target.getComponent<eol::RenderComponent>()) {
        const float healthRatio = clampf(
            enemy->getHealth() / std::max(1.f, enemy->getMaxHealth()),
            0.f,
            1.f);
        const std::uint8_t green = static_cast<std::uint8_t>(100.f + 155.f * healthRatio);
        const std::uint8_t blue = static_cast<std::uint8_t>(120.f + 110.f * healthRatio);
        render->setTint(sf::Color(255, green, blue, 200));
    }

    return true;
}

void CombatSystem::applyPlayerDamage(Entity& attacker, Entity& target, float damage) {
    if (damage <= 0.f) {
        return;
    }

    auto* player = target.getComponent<eol::PlayerComponent>();
    if (!player) {
        return;
    }

    if (player->isInvulnerable()) {
        return;
    }

    // Slightly reduce damage if attacker is not hostile (future use)
    if (!attacker.getComponent<eol::EnemyComponent>()) {
        damage *= 0.8f;
    }

    player->applyDamage(damage);

    if (auto* render = target.getComponent<eol::RenderComponent>()) {
        render->setTint(sf::Color(255, 160, 160, 220));
    }
}

Entity* CombatSystem::findPlayer(const std::vector<Entity*>& entities) const {
    for (Entity* entity : entities) {
        if (entity && entity->name == "Player") {
            return entity;
        }
    }
    return nullptr;
}

