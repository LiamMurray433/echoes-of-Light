#include "components/PuzzleComponent.h"

namespace eol {

PuzzleComponent::PuzzleComponent()
    : Component("Puzzle") {}

unsigned int PuzzleComponent::getRequiredLight() const noexcept {
    return m_requiredLight;
}

void PuzzleComponent::setRequiredLight(unsigned int requiredLight) noexcept {
    m_requiredLight = requiredLight;
}

float PuzzleComponent::getAccumulatedLight() const noexcept {
    return m_receivedLight;
}

void PuzzleComponent::setAccumulatedLight(float value) noexcept {
    m_receivedLight = value;
}

void PuzzleComponent::addReceivedLight(float value) noexcept {
    m_receivedLight += value;
}

bool PuzzleComponent::isSolved() const noexcept {
    return m_solved;
}

void PuzzleComponent::setSolved(bool solved) noexcept {
    m_solved = solved;
}

} // namespace eol

