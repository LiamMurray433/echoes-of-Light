#include "Systems.h"

#include "components/AnimationComponent.h"
#include "components/LightEmitterComponent.h"
#include "components/PlayerComponent.h"
#include "components/RenderComponent.h"
#include "components/TransformComponent.h"

#include <cmath>

namespace {
sf::Vector2f normalizeVector(const sf::Vector2f& value) {
    const float length = std::sqrt(value.x * value.x + value.y * value.y);
    if (length <= 0.0001f) {
        return sf::Vector2f{1.f, 0.f};
    }
    return sf::Vector2f{value.x / length, value.y / length};
}
}

void InputSystem::update(Entity& player, float deltaTime, const sf::RenderWindow& window) {
    auto* transform = player.getComponent<eol::TransformComponent>();
    auto* playerComp = player.getComponent<eol::PlayerComponent>();
    auto* animation = player.getComponent<eol::AnimationComponent>();

    if (!transform || !playerComp) {
        return;
    }

    playerComp->tickInvulnerability(deltaTime);

    sf::Vector2f movement = getMovementInput();
    const bool isMoving = (movement.x != 0.f || movement.y != 0.f);

    if (animation) {
        animation->setAnimation(isMoving ? "walk" : "idle");
    }

    if (isMoving) {
        movement = normalizeVector(movement);

        const float speed = playerComp->getMovementSpeed();
        sf::Vector2f pos = transform->getPosition();
        pos.x += movement.x * speed * deltaTime;
        pos.y += movement.y * speed * deltaTime;

        if (pos.x < 0.f) pos.x = 0.f;
        if (pos.y < 0.f) pos.y = 0.f;
        if (pos.x > 736.f) pos.x = 736.f;
        if (pos.y > 536.f) pos.y = 536.f;

        transform->setPosition(pos);
    }

    updatePlayerEmitter(player, window);
}

sf::Vector2f InputSystem::getMovementInput() const {
    sf::Vector2f movement(0.f, 0.f);

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
        movement.y -= 1.f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
        movement.y += 1.f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
        movement.x -= 1.f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
        movement.x += 1.f;
    }

    return movement;
}

void InputSystem::updatePlayerEmitter(Entity& player, const sf::RenderWindow& window) {
    auto* emitter = player.getComponent<eol::LightEmitterComponent>();
    auto* transform = player.getComponent<eol::TransformComponent>();
    if (!emitter || !transform) {
        return;
    }

    sf::Vector2f origin = transform->getPosition();
    if (auto* render = player.getComponent<eol::RenderComponent>()) {
        const sf::Sprite& sprite = render->getSprite();
        origin = sprite.getTransform().transformPoint(sprite.getOrigin());
    }
    const sf::Vector2f cursor = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::Vector2f aimDir = cursor - origin;
    aimDir = normalizeVector(aimDir);
    emitter->setDirection(aimDir);

    const bool triggerHeld =
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) ||
        sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    emitter->setTriggerHeld(triggerHeld);
}

