#pragma once

#include "components/Component.h"

#include <cstdint>

namespace eol {

class PlayerComponent : public Component {
public:
    PlayerComponent();

    float getMovementSpeed() const noexcept;
    void setMovementSpeed(float speed) noexcept;

    float getLightCapacity() const noexcept;
    void setLightCapacity(float capacity) noexcept;

    std::uint32_t getFragments() const noexcept;
    void setFragments(std::uint32_t count) noexcept;

    std::uint32_t getUpgradePoints() const noexcept;
    void setUpgradePoints(std::uint32_t points) noexcept;

    float getHealth() const noexcept;
    float getMaxHealth() const noexcept;
    void setHealth(float health) noexcept;
    void setMaxHealth(float maxHealth) noexcept;
    void applyDamage(float amount) noexcept;
    bool isAlive() const noexcept;
    void setInvulnerability(float duration) noexcept;
    bool isInvulnerable() const noexcept;
    void tickInvulnerability(float deltaTime) noexcept;

private:
    float m_movementSpeed{120.f};
    float m_lightCapacity{100.f};
    std::uint32_t m_lightFragments{0};
    std::uint32_t m_upgradePoints{0};
    float m_health{100.f};
    float m_maxHealth{100.f};
    float m_invulnerabilityTimer{0.f};
};

} // namespace eol

