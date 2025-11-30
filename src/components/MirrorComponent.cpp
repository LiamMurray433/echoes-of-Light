#include "components/MirrorComponent.h"

#include <cmath>

namespace {
sf::Vector2f normalizeVector(const sf::Vector2f& value) {
    const float length = std::sqrt(value.x * value.x + value.y * value.y);
    if (length <= 0.0001f) {
        return sf::Vector2f{0.f, -1.f};
    }
    return sf::Vector2f{ value.x / length, value.y / length };
}
} // namespace

namespace eol {

MirrorComponent::MirrorComponent()
    : Component("Mirror")
    , m_normal(0.f, -1.f)
    , m_size(42.f, 12.f)
    , m_reflectionLoss(0.1f)
    , m_type(MirrorType::Flat)
    , m_active(true) {}

void MirrorComponent::setNormal(const sf::Vector2f& normal) noexcept {
    m_normal = normalizeVector(normal);
}

const sf::Vector2f& MirrorComponent::getNormal() const noexcept {
    return m_normal;
}

void MirrorComponent::setSize(const sf::Vector2f& size) noexcept {
    m_size = size;
}

const sf::Vector2f& MirrorComponent::getSize() const noexcept {
    return m_size;
}

void MirrorComponent::setReflectionLoss(float loss) noexcept {
    m_reflectionLoss = loss;
}

float MirrorComponent::getReflectionLoss() const noexcept {
    return m_reflectionLoss;
}

void MirrorComponent::setType(MirrorType type) noexcept {
    m_type = type;
}

MirrorComponent::MirrorType MirrorComponent::getType() const noexcept {
    return m_type;
}

void MirrorComponent::setActive(bool active) noexcept {
    m_active = active;
}

bool MirrorComponent::isActive() const noexcept {
    return m_active;
}

} // namespace eol


