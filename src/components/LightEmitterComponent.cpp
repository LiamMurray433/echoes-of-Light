#include "components/LightEmitterComponent.h"

#include <algorithm>
#include <cmath>

namespace {
sf::Vector2f normalize(const sf::Vector2f& vec) {
    const float length = std::sqrt(vec.x * vec.x + vec.y * vec.y);
    if (length <= 0.0001f) {
        return sf::Vector2f{1.f, 0.f};
    }
    return sf::Vector2f{vec.x / length, vec.y / length};
}
} // namespace

namespace eol {

LightEmitterComponent::LightEmitterComponent()
    : Component("LightEmitter")
    , m_direction(1.f, 0.f)
    , m_beamColor(255, 230, 160, 255)
    , m_beamLength(520.f)
    , m_beamWidth(10.f)
    , m_damage(35.f)
    , m_cooldown(0.18f)
    , m_beamDuration(0.12f)
    , m_energyCost(4.f)
    , m_cooldownTimer(0.f)
    , m_maxReflections(3)
    , m_triggerHeld(false)
    , m_continuous(true) {}

void LightEmitterComponent::setDirection(const sf::Vector2f& direction) noexcept {
    m_direction = normalize(direction);
}

const sf::Vector2f& LightEmitterComponent::getDirection() const noexcept {
    return m_direction;
}

void LightEmitterComponent::setBeamLength(float length) noexcept {
    m_beamLength = length;
}

float LightEmitterComponent::getBeamLength() const noexcept {
    return m_beamLength;
}

void LightEmitterComponent::setBeamWidth(float width) noexcept {
    m_beamWidth = width;
}

float LightEmitterComponent::getBeamWidth() const noexcept {
    return m_beamWidth;
}

void LightEmitterComponent::setDamage(float damage) noexcept {
    m_damage = damage;
}

float LightEmitterComponent::getDamage() const noexcept {
    return m_damage;
}

void LightEmitterComponent::setCooldown(float cooldown) noexcept {
    m_cooldown = cooldown;
}

float LightEmitterComponent::getCooldown() const noexcept {
    return m_cooldown;
}

void LightEmitterComponent::setBeamDuration(float duration) noexcept {
    m_beamDuration = duration;
}

float LightEmitterComponent::getBeamDuration() const noexcept {
    return m_beamDuration;
}

void LightEmitterComponent::setMaxReflections(std::uint32_t reflections) noexcept {
    m_maxReflections = reflections;
}

std::uint32_t LightEmitterComponent::getMaxReflections() const noexcept {
    return m_maxReflections;
}

void LightEmitterComponent::setTriggerHeld(bool held) noexcept {
    m_triggerHeld = held;
}

bool LightEmitterComponent::isTriggerHeld() const noexcept {
    return m_triggerHeld;
}

void LightEmitterComponent::setContinuousFire(bool continuous) noexcept {
    m_continuous = continuous;
}

bool LightEmitterComponent::usesContinuousFire() const noexcept {
    return m_continuous;
}

void LightEmitterComponent::setBeamColor(const sf::Color& color) noexcept {
    m_beamColor = color;
}

const sf::Color& LightEmitterComponent::getBeamColor() const noexcept {
    return m_beamColor;
}

void LightEmitterComponent::setEnergyCost(float cost) noexcept {
    m_energyCost = cost;
}

float LightEmitterComponent::getEnergyCost() const noexcept {
    return m_energyCost;
}

void LightEmitterComponent::advanceCooldown(float deltaTime) noexcept {
    if (m_cooldownTimer > 0.f) {
        m_cooldownTimer = std::max(0.f, m_cooldownTimer - deltaTime);
    }
}

bool LightEmitterComponent::canFire() const noexcept {
    return m_cooldownTimer <= 0.f;
}

void LightEmitterComponent::registerShot() noexcept {
    m_cooldownTimer = m_cooldown;
}

} // namespace eol


