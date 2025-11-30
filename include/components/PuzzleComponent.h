#pragma once

#include "components/Component.h"

namespace eol {

class PuzzleComponent : public Component {
public:
    PuzzleComponent();

    unsigned int getRequiredLight() const noexcept;
    void setRequiredLight(unsigned int requiredLight) noexcept;

    float getAccumulatedLight() const noexcept;
    void setAccumulatedLight(float value) noexcept;
    void addReceivedLight(float value) noexcept;

    bool isSolved() const noexcept;
    void setSolved(bool solved) noexcept;

private:
    unsigned int m_requiredLight{1};
    float m_receivedLight{0.f};
    bool m_solved{false};
};

} // namespace eol

