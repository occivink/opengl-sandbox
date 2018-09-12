#pragma once

#include "input.hpp"
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <variant>
#include <vector>
#include <queue>

struct Viewport {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

class Camera {
public:
    const glm::mat4& projection_view() const;

    void draw_widget();

    void handle_input(Input& i);

    void set_position(glm::vec3 pos) { m_pos = std::move(pos); m_dirty = true; }
    void set_viewport(Viewport v);

    void render(const Camera& cam) const;

    const glm::vec3& position() const { return m_pos; }
    const glm::quat& rotation() const { return m_rot; }
    const Viewport& viewport() const { return m_viewport; }

private:
    glm::vec3 m_pos = glm::vec3(0.0f);
    glm::quat m_rot = glm::quat(1,0,0,0);

    bool m_perspective = true;
    int m_fov = 70;
    int m_scale = 1;
    bool m_lock_aspect = true;
    float m_aspect = 16.f/9.f;
    float m_near = 0.1f;
    float m_far = 100.f;

    bool m_fps = true;
    float m_distance = 5.f; // only in centered mode

    bool m_visible_to_others = true;

    Viewport m_viewport;

    mutable bool m_dirty = true;
    mutable glm::mat4 m_projection_view;
};

struct ScreenPartition {
    enum Layout {
        HORIZONTAL,
        VERTICAL,
    };
    struct Container {
        Layout layout;
        std::vector<std::variant<Container, Camera*>> children;
    };
    int padding = 5;
    std::vector<Camera> all_cam;
    Viewport viewport;
    Container main;

    void grid();
    void sidebyside();
    void horizontal();
    void single();

    void set_viewport(Viewport v);

    void process(Container& c, Viewport v);

    void draw_delimiters();

};
