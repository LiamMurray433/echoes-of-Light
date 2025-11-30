#include "components/LightComponent.h"

#include <algorithm>

namespace eol {

LightComponent::LightComponent()
    : Component("Light") {
    m_baseIntensity = m_intensity;
}

void LightComponent::setRadius(float radius) noexcept {
    m_radius = radius;
}

float LightComponent::getRadius() const noexcept {
    return m_radius;
}

void LightComponent::setIntensity(float intensity) noexcept {
    m_intensity = intensity;
}

float LightComponent::getIntensity() const noexcept {
    return m_intensity;
}

void LightComponent::setBaseIntensity(float intensity) noexcept {
    m_baseIntensity = std::max(0.f, intensity);
    m_intensity = m_baseIntensity;
    resetBoostTimer();
}

float LightComponent::getBaseIntensity() const noexcept {
    return m_baseIntensity;
}

void LightComponent::setDecayRate(float rate) noexcept {
    m_decayRate = std::max(0.f, rate);
}

float LightComponent::getDecayRate() const noexcept {
    return m_decayRate;
}

void LightComponent::setDecayDelay(float delay) noexcept {
    m_decayDelay = std::max(0.f, delay);
}

float LightComponent::getDecayDelay() const noexcept {
    return m_decayDelay;
}

void LightComponent::resetBoostTimer() noexcept {
    m_timeSinceBoost = 0.f;
}

void LightComponent::advanceBoostTimer(float deltaTime) noexcept {
    m_timeSinceBoost += deltaTime;
}

float LightComponent::getBoostTimer() const noexcept {
    return m_timeSinceBoost;
}

void LightComponent::setWeaponized(bool weaponized) noexcept {
    m_weaponized = weaponized;
}

bool LightComponent::isWeaponized() const noexcept {
    return m_weaponized;
}

} // namespace eol

