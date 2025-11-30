#include "Systems.h"

#include "components/AnimationComponent.h"

void AnimationSystem::update(std::vector<Entity*>& entities, float deltaTime) {
    for (Entity* entity : entities) {
        if (!entity) continue;

        auto* animation = entity->getComponent<eol::AnimationComponent>();
        if (animation && animation->isEnabled()) {
            animation->update(deltaTime);
        }
    }
}

