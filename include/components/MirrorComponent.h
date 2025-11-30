#pragma once

#include <SFML/Graphics.hpp>

#include "components/Component.h"

namespace eol {

class MirrorComponent : public Component {
public:
    enum class MirrorType {
        Flat,
        Splitter,
        Prism
    };

    MirrorComponent();

    void setNormal(const sf::Vector2f& normal) noexcept;
    const sf::Vector2f& getNormal() const noexcept;

    void setSize(const sf::Vector2f& size) noexcept;
    const sf::Vector2f& getSize() const noexcept;

    void setReflectionLoss(float loss) noexcept;
    float getReflectionLoss() const noexcept;

    void setType(MirrorType type) noexcept;
    MirrorType getType() const noexcept;

    void setActive(bool active) noexcept;
    bool isActive() const noexcept;

private:
    sf::Vector2f m_normal;
    sf::Vector2f m_size;
    float m_reflectionLoss;
    MirrorType m_type;
    bool m_active;
};

} // namespace eol


