#include "Systems.h"

#include "components/AnimationComponent.h"
#include "components/RenderComponent.h"
#include "components/TransformComponent.h"

void RenderSystem::render(sf::RenderWindow& window, std::vector<Entity*>& entities) {
    for (Entity* entity : entities) {
        if (!entity) continue;

        auto* render = entity->getComponent<eol::RenderComponent>();
        if (!render || !render->isEnabled()) {
            continue;
        }

        sf::Sprite& sprite = render->getSprite();
        updateSpriteFromComponents(sprite, *entity);
        window.draw(sprite);
    }
}

void RenderSystem::updateSpriteFromComponents(sf::Sprite& sprite, Entity& entity) {
    if (auto* transform = entity.getComponent<eol::TransformComponent>()) {
        sprite.setPosition(transform->getPosition());
        sprite.setScale(transform->getScale());
        sprite.setRotation(sf::degrees(transform->getRotation()));
    }

    if (auto* animation = entity.getComponent<eol::AnimationComponent>()) {
        if (const sf::Texture* texture = animation->getCurrentTexture()) {
            sprite.setTexture(*texture, false);
            sprite.setTextureRect(animation->getCurrentFrameRect());
        }
    }

    if (auto* render = entity.getComponent<eol::RenderComponent>()) {
        sprite.setColor(render->getTint());
    }
}

