#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>

#include "components/Component.h"

namespace eol {

class LightEmitterComponent : public Component {
public:
    LightEmitterComponent();

    void setDirection(const sf::Vector2f& direction) noexcept;
    const sf::Vector2f& getDirection() const noexcept;

    void setBeamLength(float length) noexcept;
    float getBeamLength() const noexcept;

    void setBeamWidth(float width) noexcept;
    float getBeamWidth() const noexcept;

    void setDamage(float damage) noexcept;
    float getDamage() const noexcept;

    void setCooldown(float cooldown) noexcept;
    float getCooldown() const noexcept;

    void setBeamDuration(float duration) noexcept;
    float getBeamDuration() const noexcept;

    void setMaxReflections(std::uint32_t reflections) noexcept;
    std::uint32_t getMaxReflections() const noexcept;

    void setTriggerHeld(bool held) noexcept;
    bool isTriggerHeld() const noexcept;

    void setContinuousFire(bool continuous) noexcept;
    bool usesContinuousFire() const noexcept;

    void setBeamColor(const sf::Color& color) noexcept;
    const sf::Color& getBeamColor() const noexcept;

    void setEnergyCost(float cost) noexcept;
    float getEnergyCost() const noexcept;

    void advanceCooldown(float deltaTime) noexcept;
    bool canFire() const noexcept;
    void registerShot() noexcept;

private:
    sf::Vector2f m_direction;
    sf::Color m_beamColor;
    float m_beamLength;
    float m_beamWidth;
    float m_damage;
    float m_cooldown;
    float m_beamDuration;
    float m_energyCost;
    float m_cooldownTimer;
    std::uint32_t m_maxReflections;
    bool m_triggerHeld;
    bool m_continuous;
};

} // namespace eol


