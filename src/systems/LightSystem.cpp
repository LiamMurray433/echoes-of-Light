#include "Systems.h"

#include "components/LightComponent.h"
#include "components/LightEmitterComponent.h"
#include "components/LightSourceComponent.h"
#include "components/MirrorComponent.h"
#include "components/PuzzleComponent.h"
#include "components/RenderComponent.h"
#include "components/TransformComponent.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
constexpr float kEpsilon = 0.0001f;

sf::Vector2f normalizeVector(const sf::Vector2f& value) {
    const float length = std::sqrt(value.x * value.x + value.y * value.y);
    if (length <= kEpsilon) {
        return sf::Vector2f{1.f, 0.f};
    }
    return sf::Vector2f{value.x / length, value.y / length};
}

float clampf(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

float dot(const sf::Vector2f& lhs, const sf::Vector2f& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

sf::Vector2f perpendicular(const sf::Vector2f& v) {
    return sf::Vector2f{-v.y, v.x};
}

sf::Vector2f rectCenter(const sf::FloatRect& rect) {
    return sf::Vector2f{rect.position.x + rect.size.x * 0.5f,
                        rect.position.y + rect.size.y * 0.5f};
}

sf::Vector2f rotateVector(const sf::Vector2f& v, float degrees) {
    const float radians = degrees * 3.1415926535f / 180.f;
    const float cs = std::cos(radians);
    const float sn = std::sin(radians);
    return sf::Vector2f{v.x * cs - v.y * sn, v.x * sn + v.y * cs};
}
} // namespace

LightSystem::LightSystem(CombatSystem& combatSystem)
    : m_beamSegments()
    , m_darknessOverlay()
    , m_debugOverlay(false)
    , m_ambientLight(0.28f)
    , m_debugMirrorBounds()
    , m_debugHitPoints()
    , m_combat(combatSystem) {
    m_darknessOverlay.setFillColor(sf::Color(0, 0, 0, 200));
}

void LightSystem::setAmbientLight(float ambient) noexcept {
    m_ambientLight = clampf(ambient, 0.f, 1.f);
}

float LightSystem::getAmbientLight() const noexcept {
    return m_ambientLight;
}

void LightSystem::setDebugOverlayEnabled(bool enabled) noexcept {
    m_debugOverlay = enabled;
}

bool LightSystem::isDebugOverlayEnabled() const noexcept {
    return m_debugOverlay;
}

void LightSystem::update(std::vector<Entity*>& entities, float deltaTime, const sf::RenderWindow& window) {
    refreshBeamTimers(deltaTime);
    updateEmitters(entities, deltaTime, window);
    updateLightFields(entities, deltaTime);
}

void LightSystem::render(sf::RenderTarget& target, std::vector<Entity*>& entities) {
    ensureOverlaySize(target);
    drawLightGlows(target, entities);
    drawOverlay(target);
    drawBeams(target);
    if (m_debugOverlay) {
        drawDebugData(target);
    }
}

void LightSystem::refreshBeamTimers(float deltaTime) {
    for (auto& beam : m_beamSegments) {
        beam.ttl = std::max(0.f, beam.ttl - deltaTime);
    }

    m_beamSegments.erase(
        std::remove_if(
            m_beamSegments.begin(),
            m_beamSegments.end(),
            [](const BeamSegment& beam) { return beam.ttl <= 0.f; }),
        m_beamSegments.end());
}

void LightSystem::updateEmitters(std::vector<Entity*>& entities,
                                 float deltaTime,
                                 const sf::RenderWindow& window) {
    auto refreshDebugBounds = [this, &entities]() {
        if (!m_debugOverlay) {
            m_debugMirrorBounds.clear();
            return;
        }

        m_debugMirrorBounds.clear();
        for (Entity* entity : entities) {
            if (!entity) continue;
            if (auto bounds = computeMirrorBounds(*entity)) {
                m_debugMirrorBounds.push_back(*bounds);
            }
        }
    };

    refreshDebugBounds();
    m_debugHitPoints.clear();

    struct PendingShot {
        Entity* owner;
        eol::LightEmitterComponent* emitter;
        sf::Vector2f origin;
    };

    std::vector<PendingShot> readyShots;
    readyShots.reserve(entities.size());

    for (Entity* entity : entities) {
        if (!entity) continue;

        auto* emitter = entity->getComponent<eol::LightEmitterComponent>();
        auto* transform = entity->getComponent<eol::TransformComponent>();
        if (!emitter || !transform || !emitter->isEnabled()) {
            continue;
        }

        emitter->advanceCooldown(deltaTime);

        if (!emitter->isTriggerHeld() || !emitter->canFire()) {
            continue;
        }

        sf::Vector2f origin = transform->getPosition();
        if (auto bounds = computeBounds(*entity)) {
            origin = rectCenter(*bounds);
        }

        readyShots.push_back(PendingShot{entity, emitter, origin});
    }

    for (const PendingShot& shot : readyShots) {
        emitBeam(*shot.owner, *shot.emitter, entities, shot.origin, shot.emitter->getDirection());
    }
}

void LightSystem::emitBeam(Entity& owner,
                           eol::LightEmitterComponent& emitter,
                           std::vector<Entity*>& entities,
                           const sf::Vector2f& origin,
                           const sf::Vector2f& direction) {
    castBeam(owner,
             entities,
             origin,
             normalizeVector(direction),
             emitter.getBeamLength(),
             emitter.getBeamWidth(),
             emitter.getBeamColor(),
             emitter.getDamage(),
             emitter.getBeamDuration(),
             emitter.getMaxReflections());
    emitter.registerShot();
}

void LightSystem::castBeam(Entity& owner,
                           std::vector<Entity*>& entities,
                           const sf::Vector2f& origin,
                           const sf::Vector2f& direction,
                           float range,
                           float width,
                           sf::Color color,
                           float intensity,
                           float ttl,
                           std::uint32_t reflectionsLeft) {
    sf::Vector2f currentStart = origin;
    sf::Vector2f currentDirection = normalizeVector(direction);
    float remainingRange = range;
    float currentIntensity = intensity;

    while (remainingRange > 4.f && currentIntensity > 0.1f) {
        float nearestDistance = remainingRange;
        Entity* hitEntity = nullptr;
        sf::Vector2f hitNormal{0.f, 0.f};
        eol::MirrorComponent* hitMirror = nullptr;

        for (Entity* candidate : entities) {
            if (!candidate || candidate == &owner) {
                continue;
            }

            float hitDistance = 0.f;
            sf::Vector2f normal{};

            if (auto* mirror = candidate->getComponent<eol::MirrorComponent>(); mirror && mirror->isActive()) {
                if (rayIntersectsMirror(currentStart, currentDirection, remainingRange, *candidate, hitDistance, normal)) {
                    if (hitDistance < nearestDistance) {
                        nearestDistance = hitDistance;
                        hitEntity = candidate;
                        hitNormal = normal;
                        hitMirror = mirror;
                    }
                }
                continue;
            }

            auto bounds = computeBounds(*candidate);
            if (!bounds) {
                continue;
            }

            if (rayIntersectsRect(currentStart, currentDirection, remainingRange, *bounds, hitDistance, normal)) {
                if (hitDistance < nearestDistance) {
                    nearestDistance = hitDistance;
                    hitEntity = candidate;
                    hitNormal = normal;
                    hitMirror = nullptr;
                }
            }
        }

        sf::Vector2f endPoint = currentStart + currentDirection * nearestDistance;
        m_beamSegments.push_back(BeamSegment{
            currentStart,
            endPoint,
            color,
            width,
            ttl,
            ttl,
            currentIntensity });

        if (!hitEntity) {
            break;
        }

        if (hitMirror) {
            if (hitMirror->getType() == eol::MirrorComponent::MirrorType::Flat && reflectionsLeft > 0) {
                sf::Vector2f mirrorNormal = hitNormal;
                const float loss = clampf(hitMirror->getReflectionLoss(), 0.f, 0.9f);
                currentIntensity *= (1.f - loss);
                reflectionsLeft--;

                remainingRange -= nearestDistance;
                currentDirection = reflect(currentDirection, mirrorNormal);
                currentStart = endPoint + currentDirection * 4.f;
                continue;
            }

            if (reflectionsLeft > 0) {
                if (hitMirror->getType() == eol::MirrorComponent::MirrorType::Splitter) {
                    const sf::Vector2f mirrorNormal = hitNormal;
                    const sf::Vector2f tangent = normalizeVector(perpendicular(mirrorNormal));
                    const float childRange = remainingRange * 0.65f;
                    const float childWidth = width * 0.7f;
                    const float childIntensity = intensity * 0.6f;
                    const float childTtl = ttl * 0.85f;

                    castBeam(owner,
                             entities,
                             endPoint + tangent * 4.f,
                             tangent,
                             childRange,
                             childWidth,
                             color,
                             childIntensity,
                             childTtl,
                             reflectionsLeft - 1);

                    castBeam(owner,
                             entities,
                             endPoint - tangent * 4.f,
                             -tangent,
                             childRange,
                             childWidth,
                             color,
                             childIntensity,
                             childTtl,
                             reflectionsLeft - 1);
                }
                else if (hitMirror->getType() == eol::MirrorComponent::MirrorType::Prism) {
                    constexpr float prismAngle = 35.f;
                    const auto dirA = normalizeVector(rotateVector(currentDirection, prismAngle));
                    const auto dirB = normalizeVector(rotateVector(currentDirection, -prismAngle));
                    const float childRange = remainingRange * 0.55f;
                    const float childIntensity = intensity * 0.5f;

                    castBeam(owner,
                             entities,
                             endPoint,
                             dirA,
                             childRange,
                             width * 0.6f,
                             color,
                             childIntensity,
                             ttl * 0.75f,
                             reflectionsLeft - 1);
                    castBeam(owner,
                             entities,
                             endPoint,
                             dirB,
                             childRange,
                             width * 0.6f,
                             color,
                             childIntensity,
                             ttl * 0.75f,
                             reflectionsLeft - 1);
                }
            }

            // Splitter/Prism consume the original beam
            break;
        }

        handleBeamImpact(owner, *hitEntity, currentIntensity, endPoint);
        if (m_debugOverlay) {
            m_debugHitPoints.push_back(endPoint);
        }
        break;
    }
}

bool LightSystem::rayIntersectsRect(const sf::Vector2f& origin,
                                    const sf::Vector2f& direction,
                                    float maxDistance,
                                    const sf::FloatRect& bounds,
                                    float& outDistance,
                                    sf::Vector2f& outNormal) const {
    const float step = 6.f;
    sf::Vector2f sample = origin;
    float travelled = 0.f;

    while (travelled <= maxDistance) {
        if (bounds.contains(sample)) {
            outDistance = travelled;

            const float leftDistance = std::abs(sample.x - bounds.position.x);
            const float rightDistance = std::abs(bounds.position.x + bounds.size.x - sample.x);
            const float topDistance = std::abs(sample.y - bounds.position.y);
            const float bottomDistance = std::abs(bounds.position.y + bounds.size.y - sample.y);
            const float minAxis = std::min(std::min(leftDistance, rightDistance), std::min(topDistance, bottomDistance));

            if (minAxis == leftDistance) {
                outNormal = sf::Vector2f{-1.f, 0.f};
            }
            else if (minAxis == rightDistance) {
                outNormal = sf::Vector2f{1.f, 0.f};
            }
            else if (minAxis == topDistance) {
                outNormal = sf::Vector2f{0.f, -1.f};
            }
            else {
                outNormal = sf::Vector2f{0.f, 1.f};
            }
            return true;
        }

        travelled += step;
        sample += direction * step;
    }

    return false;
}

bool LightSystem::rayIntersectsMirror(const sf::Vector2f& origin,
                                      const sf::Vector2f& direction,
                                      float maxDistance,
                                      Entity& mirrorEntity,
                                      float& outDistance,
                                      sf::Vector2f& outNormal) const {
    auto* mirror = mirrorEntity.getComponent<eol::MirrorComponent>();
    auto* transform = mirrorEntity.getComponent<eol::TransformComponent>();
    if (!mirror || !transform) {
        return false;
    }

    sf::Vector2f normal = normalizeVector(mirror->getNormal());
    sf::Vector2f center = transform->getPosition();

    if (auto* render = mirrorEntity.getComponent<eol::RenderComponent>()) {
        const sf::Sprite& sprite = render->getSprite();
        center = sprite.getTransform().transformPoint(sprite.getOrigin());
    }

    const float denom = dot(direction, normal);
    if (std::abs(denom) <= kEpsilon) {
        return false;
    }

    const float distanceAlongRay = dot(center - origin, normal) / denom;
    if (distanceAlongRay < 0.f || distanceAlongRay > maxDistance) {
        return false;
    }

    const sf::Vector2f hitPoint = origin + direction * distanceAlongRay;
    const sf::Vector2f tangent = normalizeVector(perpendicular(normal));
    const float halfLength = std::max(4.f, mirror->getSize().x * 0.5f);
    const float halfThickness = std::max(2.f, mirror->getSize().y * 0.5f);

    const float distAlongTangent = std::abs(dot(hitPoint - center, tangent));
    if (distAlongTangent > halfLength) {
        return false;
    }

    const float distFromPlane = std::abs(dot(hitPoint - center, normal));
    if (distFromPlane > halfThickness) {
        return false;
    }

    outDistance = distanceAlongRay;
    outNormal = normal;
    return true;
}

std::optional<sf::FloatRect> LightSystem::computeMirrorBounds(Entity& entity) const {
    auto* mirror = entity.getComponent<eol::MirrorComponent>();
    auto* transform = entity.getComponent<eol::TransformComponent>();
    if (!mirror || !mirror->isActive() || !transform) {
        return std::nullopt;
    }

    const sf::Vector2f position = transform->getPosition();
    const sf::Vector2f size = mirror->getSize();
    return sf::FloatRect({position.x - size.x * 0.5f,
                          position.y - size.y * 0.5f},
                         size);
}

std::optional<sf::FloatRect> LightSystem::computeBounds(Entity& entity) const {
    if (auto mirrorBounds = computeMirrorBounds(entity)) {
        return mirrorBounds;
    }

    if (auto* render = entity.getComponent<eol::RenderComponent>()) {
        const sf::Sprite& sprite = render->getSprite();
        return sprite.getGlobalBounds();
    }

    if (auto* transform = entity.getComponent<eol::TransformComponent>()) {
        const sf::Vector2f pos = transform->getPosition();
        constexpr float defaultSize = 32.f;
        return sf::FloatRect(sf::Vector2f{pos.x, pos.y},
                             sf::Vector2f{defaultSize, defaultSize});
    }

    return std::nullopt;
}

void LightSystem::handleBeamImpact(Entity& owner, Entity& target, float intensity, const sf::Vector2f& hitPoint) {
    applyPuzzleLight(target, intensity);
    m_combat.applyBeamHit(owner, target, intensity, hitPoint);

    if (auto* light = target.getComponent<eol::LightComponent>()) {
        const float boosted = clampf(light->getIntensity() + intensity * 0.01f, 0.f, 1.5f);
        light->setIntensity(boosted);
        light->resetBoostTimer();
    }

    if (auto* source = target.getComponent<eol::LightSourceComponent>()) {
        source->setActive(true);
        source->setFuel(std::min(100.f, source->getFuel() + intensity));
    }

    if (m_debugOverlay) {
        m_debugHitPoints.push_back(hitPoint);
    }
}

void LightSystem::applyPuzzleLight(Entity& entity, float intensity) {
    auto* puzzle = entity.getComponent<eol::PuzzleComponent>();
    if (!puzzle || puzzle->isSolved()) {
        return;
    }

    puzzle->addReceivedLight(intensity * 0.1f);
    if (puzzle->getAccumulatedLight() >= static_cast<float>(puzzle->getRequiredLight())) {
        puzzle->setSolved(true);
        std::cout << "[LightSystem] Beacon " << entity.name << " reactivated." << std::endl;
    }
}

sf::Vector2f LightSystem::reflect(const sf::Vector2f& direction, const sf::Vector2f& normal) const {
    const sf::Vector2f n = normalizeVector(normal);
    return normalizeVector(direction - 2.f * dot(direction, n) * n);
}

void LightSystem::updateLightFields(std::vector<Entity*>& entities, float deltaTime) {
    for (Entity* entity : entities) {
        if (!entity) continue;

        auto* light = entity->getComponent<eol::LightComponent>();
        auto* render = entity->getComponent<eol::RenderComponent>();
        if (!light || !render) {
            continue;
        }

        light->advanceBoostTimer(deltaTime);

        if (light->getIntensity() > light->getBaseIntensity()) {
            if (light->getBoostTimer() >= light->getDecayDelay()) {
                const float newIntensity = std::max(
                    light->getBaseIntensity(),
                    light->getIntensity() - light->getDecayRate() * deltaTime);
                light->setIntensity(newIntensity);
            }
        }
        else if (light->getIntensity() < light->getBaseIntensity()) {
            const float restoreIntensity = std::min(
                light->getBaseIntensity(),
                light->getIntensity() + light->getDecayRate() * deltaTime * 0.5f);
            light->setIntensity(restoreIntensity);
        }

        const float brightness = clampf(m_ambientLight + light->getIntensity(), 0.f, 1.25f);
        sf::Color color = render->getTint();
        color.r = static_cast<std::uint8_t>(clampf(100.f + brightness * 140.f, 0.f, 255.f));
        color.g = static_cast<std::uint8_t>(clampf(100.f + brightness * 120.f, 0.f, 255.f));
        color.b = static_cast<std::uint8_t>(clampf(110.f + brightness * 80.f, 0.f, 255.f));
        color.a = static_cast<std::uint8_t>(clampf(brightness, 0.f, 1.f) * 255.f);
        render->setTint(color);
    }
}

void LightSystem::ensureOverlaySize(const sf::RenderTarget& target) {
    const auto size = target.getSize();
    m_darknessOverlay.setSize(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
    const auto alpha = static_cast<std::uint8_t>(clampf(1.f - m_ambientLight, 0.f, 1.f) * 220.f);
    m_darknessOverlay.setFillColor(sf::Color(5, 5, 15, alpha));
}

void LightSystem::drawBeams(sf::RenderTarget& target) const {
    if (m_beamSegments.empty()) {
        return;
    }

    sf::VertexArray mesh{sf::PrimitiveType::Triangles};
    for (const auto& beam : m_beamSegments) {
        const sf::Vector2f dir = beam.end - beam.start;
        if (std::abs(dir.x) < kEpsilon && std::abs(dir.y) < kEpsilon) {
            continue;
        }

        sf::Vector2f normal = normalizeVector(perpendicular(dir)) * (beam.width * 0.5f);
        const float lifeRatio = beam.lifetime > 0.f ? clampf(beam.ttl / beam.lifetime, 0.f, 1.f) : 0.f;
        sf::Color color = beam.color;
        color.a = static_cast<std::uint8_t>(color.a * lifeRatio * clampf(beam.intensity / 50.f, 0.2f, 1.f));

        const sf::Vector2f v0 = beam.start + normal;
        const sf::Vector2f v1 = beam.start - normal;
        const sf::Vector2f v2 = beam.end + normal;
        const sf::Vector2f v3 = beam.end - normal;

        mesh.append(sf::Vertex{v0, color});
        mesh.append(sf::Vertex{v1, color});
        mesh.append(sf::Vertex{v2, color});

        mesh.append(sf::Vertex{v1, color});
        mesh.append(sf::Vertex{v3, color});
        mesh.append(sf::Vertex{v2, color});
    }

    sf::RenderStates states;
    states.blendMode = sf::BlendAdd;
    target.draw(mesh, states);
}

void LightSystem::drawOverlay(sf::RenderTarget& target) const {
    sf::RenderStates states;
    states.blendMode = sf::BlendAlpha;
    target.draw(m_darknessOverlay, states);
}

void LightSystem::drawDebugData(sf::RenderTarget& target) const {
    for (const auto& rect : m_debugMirrorBounds) {
        sf::RectangleShape box;
        box.setPosition(rect.position);
        box.setSize(rect.size);
        box.setFillColor(sf::Color(0, 0, 0, 0));
        box.setOutlineThickness(1.f);
        box.setOutlineColor(sf::Color(120, 200, 255, 140));
        target.draw(box);
    }

    sf::CircleShape marker(3.f);
    marker.setFillColor(sf::Color(255, 220, 180, 220));
    for (const auto& point : m_debugHitPoints) {
        marker.setPosition(sf::Vector2f{point.x - 3.f, point.y - 3.f});
        target.draw(marker);
    }
}

void LightSystem::drawLightGlows(sf::RenderTarget& target, std::vector<Entity*>& entities) {
    sf::RenderStates addState;
    addState.blendMode = sf::BlendAdd;

    for (Entity* entity : entities) {
        if (!entity) continue;

        auto* light = entity->getComponent<eol::LightComponent>();
        if (!light || !light->isEnabled()) {
            continue;
        }

        const float intensity = clampf(light->getIntensity(), 0.f, 2.f);
        const float radius = std::max(36.f, light->getRadius() * 0.55f);
        auto bounds = computeBounds(*entity);

        sf::Vector2f center;
        if (bounds) {
            center = rectCenter(*bounds);
        }
        else if (auto* transform = entity->getComponent<eol::TransformComponent>()) {
            center = transform->getPosition();
        }
        else {
            continue;
        }

        const auto outerAlpha = static_cast<std::uint8_t>(clampf(intensity * 80.f + 20.f, 25.f, 160.f));

        sf::CircleShape outer(radius);
        const sf::Vector2f outerOrigin{radius, radius};
        outer.setOrigin(outerOrigin);
        outer.setPosition(center);
        outer.setFillColor(sf::Color(255, 244, 214, outerAlpha));
        target.draw(outer, addState);
    }
}

