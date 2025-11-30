#include "components/EnemyComponent.h"

#include <algorithm>

namespace eol {

EnemyComponent::EnemyComponent()
    : Component("Enemy") {}

float EnemyComponent::getResistance() const noexcept {
    return m_resistance;
}

void EnemyComponent::setResistance(float resistance) noexcept {
    m_resistance = resistance;
}

float EnemyComponent::getHealth() const noexcept {
    return m_health;
}

float EnemyComponent::getMaxHealth() const noexcept {
    return m_maxHealth;
}

void EnemyComponent::setHealth(float health) noexcept {
    m_health = std::clamp(health, 0.f, m_maxHealth);
}

void EnemyComponent::setMaxHealth(float maxHealth) noexcept {
    m_maxHealth = std::max(1.f, maxHealth);
    m_health = std::min(m_health, m_maxHealth);
}

void EnemyComponent::applyDamage(float amount) noexcept {
    if (amount <= 0.f) {
        return;
    }
    m_health = std::max(0.f, m_health - amount);
}

bool EnemyComponent::isAlive() const noexcept {
    return m_health > 0.01f;
}

float EnemyComponent::getAwarenessRadius() const noexcept {
    return m_awarenessRadius;
}

void EnemyComponent::setAwarenessRadius(float radius) noexcept {
    m_awarenessRadius = radius;
}

bool EnemyComponent::blocksLight() const noexcept {
    return m_blocksLight;
}

void EnemyComponent::setBlocksLight(bool blocksLight) noexcept {
    m_blocksLight = blocksLight;
}

} // namespace eol

