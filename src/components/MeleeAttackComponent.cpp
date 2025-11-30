#include "components/MeleeAttackComponent.h"

#include <algorithm>

namespace eol {

MeleeAttackComponent::MeleeAttackComponent()
    : Component("MeleeAttack") {}

void MeleeAttackComponent::setDamage(float damage) noexcept {
    m_damage = std::max(0.f, damage);
}

float MeleeAttackComponent::getDamage() const noexcept {
    return m_damage;
}

void MeleeAttackComponent::setRange(float range) noexcept {
    m_range = std::max(0.f, range);
}

float MeleeAttackComponent::getRange() const noexcept {
    return m_range;
}

void MeleeAttackComponent::setCooldown(float cooldown) noexcept {
    m_cooldown = std::max(0.f, cooldown);
}

float MeleeAttackComponent::getCooldown() const noexcept {
    return m_cooldown;
}

void MeleeAttackComponent::advanceCooldown(float deltaTime) noexcept {
    if (m_cooldownTimer > 0.f) {
        m_cooldownTimer = std::max(0.f, m_cooldownTimer - deltaTime);
    }
}

bool MeleeAttackComponent::canAttack() const noexcept {
    return m_cooldownTimer <= 0.f;
}

void MeleeAttackComponent::resetCooldown() noexcept {
    m_cooldownTimer = m_cooldown;
}

} // namespace eol


