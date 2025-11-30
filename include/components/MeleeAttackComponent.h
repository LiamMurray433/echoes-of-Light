#pragma once

#include "components/Component.h"

namespace eol {

class MeleeAttackComponent : public Component {
public:
    MeleeAttackComponent();

    void setDamage(float damage) noexcept;
    float getDamage() const noexcept;

    void setRange(float range) noexcept;
    float getRange() const noexcept;

    void setCooldown(float cooldown) noexcept;
    float getCooldown() const noexcept;

    void advanceCooldown(float deltaTime) noexcept;
    bool canAttack() const noexcept;
    void resetCooldown() noexcept;

private:
    float m_damage{18.f};
    float m_range{48.f};
    float m_cooldown{1.2f};
    float m_cooldownTimer{0.f};
};

} // namespace eol


