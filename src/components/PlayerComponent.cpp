#include "components/PlayerComponent.h"

#include <algorithm>

namespace eol {

PlayerComponent::PlayerComponent()
    : Component("Player") {}

float PlayerComponent::getMovementSpeed() const noexcept {
    return m_movementSpeed;
}

void PlayerComponent::setMovementSpeed(float speed) noexcept {
    m_movementSpeed = speed;
}

float PlayerComponent::getLightCapacity() const noexcept {
    return m_lightCapacity;
}

void PlayerComponent::setLightCapacity(float capacity) noexcept {
    m_lightCapacity = capacity;
}

std::uint32_t PlayerComponent::getFragments() const noexcept {
    return m_lightFragments;
}

void PlayerComponent::setFragments(std::uint32_t count) noexcept {
    m_lightFragments = count;
}

std::uint32_t PlayerComponent::getUpgradePoints() const noexcept {
    return m_upgradePoints;
}

void PlayerComponent::setUpgradePoints(std::uint32_t points) noexcept {
    m_upgradePoints = points;
}

float PlayerComponent::getHealth() const noexcept {
    return m_health;
}

float PlayerComponent::getMaxHealth() const noexcept {
    return m_maxHealth;
}

void PlayerComponent::setHealth(float health) noexcept {
    m_health = std::clamp(health, 0.f, m_maxHealth);
}

void PlayerComponent::setMaxHealth(float maxHealth) noexcept {
    m_maxHealth = std::max(1.f, maxHealth);
    m_health = std::min(m_health, m_maxHealth);
}

void PlayerComponent::applyDamage(float amount) noexcept {
    if (amount <= 0.f || m_invulnerabilityTimer > 0.f) {
        return;
    }
    m_health = std::max(0.f, m_health - amount);
    m_invulnerabilityTimer = 0.6f;
}

bool PlayerComponent::isAlive() const noexcept {
    return m_health > 0.01f;
}

void PlayerComponent::setInvulnerability(float duration) noexcept {
    m_invulnerabilityTimer = std::max(0.f, duration);
}

bool PlayerComponent::isInvulnerable() const noexcept {
    return m_invulnerabilityTimer > 0.f;
}

void PlayerComponent::tickInvulnerability(float deltaTime) noexcept {
    if (m_invulnerabilityTimer > 0.f) {
        m_invulnerabilityTimer = std::max(0.f, m_invulnerabilityTimer - deltaTime);
    }
}

} // namespace eol

