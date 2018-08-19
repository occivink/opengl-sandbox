#pragma once

#include <mat4x4.hpp>
#include <gtc/matrix_transform.hpp>

class Camera {
public:
    void look_at(glm::vec3 position, glm::vec3 look_at, glm::vec3 up_direction) {
        m_view = glm::lookAt(
            position,
            look_at,
            up_direction
        );
        m_dirty = true;
    }

    void set_perspective(float fov, float aspect, float near, float far) {
        m_projection = glm::perspective(glm::radians(fov), aspect, near, far);
        m_dirty = true;
    }

    const glm::mat4& view() const {
        return m_view;
    }

    const glm::mat4& projection() const {
        return m_projection;
    }

    const glm::mat4& projection_view() const {
        if (m_dirty) {
            m_projection_view = m_projection * m_view;
            m_dirty = false;
        }
        return m_projection_view;
    }

private:
    glm::mat4 m_view;
    glm::mat4 m_projection;
    mutable bool m_dirty = true;
    mutable glm::mat4 m_projection_view;
};
