#include "components/Component.h"
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <string>
#include <map>

namespace eol {

    // Represents a single animation (e.g., "idle", "walk", "attack")
    struct Animation {
        std::string name;
        const sf::Texture* texture;  // Pointer to the sprite sheet
        int frameCount;              // Number of frames in this animation
        int frameWidth;              // Width of each frame in pixels
        int frameHeight;             // Height of each frame in pixels
        float frameDuration;         // Time per frame in seconds
        bool loop;                   // Should the animation loop?

        Animation()
            : texture(nullptr)
            , frameCount(0)
            , frameWidth(0)
            , frameHeight(0)
            , frameDuration(0.15f)
            , loop(true) {
        }
    };

    class AnimationComponent : public Component {
    public:
        AnimationComponent();

        // Add an animation to this component
        void addAnimation(const std::string& name, const Animation& anim);

        // Set the current animation (e.g., "idle" -> "walk")
        void setAnimation(const std::string& name);

        // Get current animation name
        const std::string& getCurrentAnimationName() const noexcept;

        // Update animation (called by AnimationSystem each frame)
        void update(float deltaTime);

        // Get the current frame's texture rectangle
        sf::IntRect getCurrentFrameRect() const;

        // Get current animation's texture
        const sf::Texture* getCurrentTexture() const;

        // Check if animation has finished (for non-looping animations)
        bool isFinished() const noexcept;

        // Reset animation to first frame
        void reset();

    private:
        std::map<std::string, Animation> m_animations;
        std::string m_currentAnimation;

        int m_currentFrame{ 0 };
        float m_elapsedTime{ 0.f };
        bool m_finished{ false };
    };

} // namespace eol