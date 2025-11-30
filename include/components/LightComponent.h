#pragma once

#include "components/Component.h"

namespace eol {

class LightComponent : public Component {
public:
    LightComponent();

    void setRadius(float radius) noexcept;
    float getRadius() const noexcept;

    void setIntensity(float intensity) noexcept;
    float getIntensity() const noexcept;

    void setBaseIntensity(float intensity) noexcept;
    float getBaseIntensity() const noexcept;

    void setDecayRate(float rate) noexcept;
    float getDecayRate() const noexcept;

    void setDecayDelay(float delay) noexcept;
    float getDecayDelay() const noexcept;

    void resetBoostTimer() noexcept;
    void advanceBoostTimer(float deltaTime) noexcept;
    float getBoostTimer() const noexcept;

    void setWeaponized(bool weaponized) noexcept;
    bool isWeaponized() const noexcept;

private:
    float m_radius{160.f};
    float m_intensity{1.f};
    float m_baseIntensity{1.f};
    float m_decayRate{0.45f};
    float m_decayDelay{0.35f};
    float m_timeSinceBoost{10.f};
    bool m_weaponized{false};
};

} // namespace eol

