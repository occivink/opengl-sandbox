#pragma once

#include <mat4x4.hpp>
#include "input.hpp"

#include <optional>

class Manipulator {
public:
    enum Mode {
        Inactive,
        Translation,
        Rotation,
    };

    Manipulator();

    void render(glm::mat4 mvp) const;

    bool handleInput(const glm::mat4& mv,
                     Input& input);

    Mode mode() const { return m_mode; }
    void set_mode(Mode m) { m_mode = m; m_old_state.reset(); }

private:
    Mode m_mode = Translation;

    glm::mat4 m_model = glm::mat4(1.0f);
    struct OldState {
        glm::mat4 model;
        glm::vec4 start_point; // where the first hit occured, in local space
        glm::vec4 axis;
    };
    std::optional<OldState> m_old_state;

    enum ActiveAxis {
        None,
        X,
        Y,
        Z
    } m_activeAxis;
};
