#include "camera.hpp"
#include "cube.hpp"
#include "imgui/imgui.h"
#include "log.hpp"

#define PI 3.1415f

using namespace glm;

const mat4& Camera::projection_view() const {
    if (m_dirty) {
        mat4 view = mat4_cast(m_rot);
        view[3][0] = m_pos[0];
        view[3][1] = m_pos[1];
        view[3][2] = m_pos[2];

        mat4 proj;
        if (m_perspective) {
            proj = perspective(radians((float)m_fov), m_aspect, m_near, m_far);
        } else {
            proj = ortho(-m_aspect * m_scale, m_aspect * m_scale, -(float)m_scale, (float)m_scale, m_near, m_far);
        }

        m_projection_view = proj * inverse(view);
        m_dirty = false;
    }
    return m_projection_view;
}

void Camera::set_viewport(Viewport v) { 
    m_viewport = std::move(v); 
    if (m_lock_aspect) {
        m_aspect = (float)m_viewport.width / m_viewport.height;
        m_dirty = true; 
    }
}

void Camera::handle_input(Input& in) {
    /*if (in.sizeChanged) {
        m_viewport.width = in.width;
        m_viewport.height = in.height;
        if (m_lock_aspect) {
            m_aspect = (float)m_viewport.width / m_viewport.height;
            m_dirty = true;
        }
    }*/
    if (in.mouseDown[0] and not in.mouseCaptured) {
        if (in.mouseDelta.x != 0 or in.mouseDelta.y != 0) {
            auto& delta = in.mouseDelta;
            m_rot = normalize(
                quat(1000, 0, -delta.x, 0)) *
                m_rot *
                normalize(quat(1000, -delta.y, 0, 0));
            m_dirty = true;
        }
        in.mouseCaptured = true;
    }
    if (m_fps and not in.keyboardCaptured) {
        auto axis = vec3(0,0,0);
        if (in.keyDown[ScanCode::S_W])
            axis += vec3(0,0,-1);
        if (in.keyDown[ScanCode::S_S])
            axis += vec3(0,0,1);
        if (in.keyDown[ScanCode::S_A])
            axis += vec3(-1,0,0);
        if (in.keyDown[ScanCode::S_D])
            axis += vec3(1,0,0);
        if (axis != vec3(0,0,0)) {
            m_pos += 0.05f * (m_rot * normalize(axis));
            m_dirty = true;
            in.keyboardCaptured= true;
        }
    }
}

void Camera::render(const Camera& cam) const
{
    Cube c;
    Camera copy = *this;
    copy.m_far = 5.f;
    copy.m_dirty = true;
    auto mvp = cam.projection_view() * glm::inverse(copy.projection_view());
    c.render(mvp);
}


void Camera::draw_widget() {
    ImGui::Begin("Camera", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::DragFloat3("Position", &m_pos[0], 0.3))
        m_dirty = true;
    if (ImGui::DragFloat4("Rotation", &m_rot[0], 0.03, -1, 1)) {
        if (m_rot == quat(0,0,0,0))
            m_rot = {1,0,0,0};
        else
            m_rot = normalize(m_rot);
        m_dirty = true;
    }

    ImGui::Spacing();
    ImGui::Columns(2, NULL, false);
    if (ImGui::Selectable("Perspective", m_perspective)) {
        if (!m_perspective) {
            m_perspective = true;
            m_dirty = true;
        }
    }
    ImGui::NextColumn();
    if (ImGui::Selectable("Orthographic", !m_perspective)) {
        if (m_perspective) {
            m_perspective = false;
            m_dirty = true;
        }
    }
    ImGui::Columns(1);
    if (m_perspective) {
        if (ImGui::DragInt("FOV", &m_fov, 1.f, 30, 130))
            m_dirty = true;
    } else {
        if (ImGui::DragInt("Scale", &m_scale, 1.f/10.f, 1, 30))
            m_dirty = true;
    }
    if (m_lock_aspect)
        ImGui::DragFloat("Aspect", &m_aspect, 0.f, m_aspect, m_aspect); // poor man's disable
    else
        if (ImGui::DragFloat("Aspect", &m_aspect, 0.01f, 0.1f, 10.f))
            m_dirty = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("Lock", &m_lock_aspect) and m_lock_aspect) {
        m_aspect = (float)m_viewport.width / m_viewport.height;
        m_dirty = true;
    }
    ImGui::Spacing();
    if (ImGui::DragFloat("Near", &m_near, 1.f, 0.001, 1.f))
        m_dirty = true;
    if (ImGui::DragFloat("Far", &m_far, 1.f, 100.f, 10000.f))
        m_dirty = true;


    ImGui::Columns(2, NULL, false);
    if (ImGui::Selectable("Centered", not m_fps))
        m_fps = false;
    ImGui::NextColumn();
    if (ImGui::Selectable("FPS", m_fps))
        m_fps = true;
    ImGui::Columns(1);

    ImGui::End();
}

void ScreenPartition::grid() {
    all_cam.resize(4);
    main = Container{
        HORIZONTAL,
        { Container{
            VERTICAL,
            { }
          },
          Container{
            VERTICAL,
            {  }
          },
        },
    };
    process(main, viewport);
}
void ScreenPartition::sidebyside() {
    all_cam.resize(3);
    main = Container{
        HORIZONTAL,
        { &all_cam[0],
          Container{
            VERTICAL,
            { &all_cam[1], &all_cam[2] }
          },
        },
    };
    process(main, viewport);
}
void ScreenPartition::horizontal() {
    all_cam.resize(2);
    main = Container{
        HORIZONTAL,
        { &all_cam[0], &all_cam[1] }
    };
    process(main, viewport);
}
void ScreenPartition::single() {
    all_cam.resize(1);
    main = Container{
        HORIZONTAL,
        { &all_cam[0] }
    };
    process(main, viewport);
}
void ScreenPartition::set_viewport(Viewport v) {
    v.x += 2*padding;
    v.y += 2*padding;
    v.width -= 4*padding;
    v.height -= 4*padding;
    process(main, std::move(v));
}
void ScreenPartition::process(Container& c, Viewport v) {
    int leftover;
    auto s = c.children.size();
    if (c.layout == HORIZONTAL) {
        v.width -= 2 * padding * (s - 1);
        leftover = v.width % s;
        v.width /= s;
        if (leftover) v.width++;
    } else {
        v.width -= 2 * padding * (s - 1);
        leftover = v.height % s;
        v.height /= s;
        if (leftover) v.height++;
    }
    for (auto& child : c.children) {
        if (auto cont = std::get_if<Container>(&child)) {
            process(*cont, v);
        } else if (auto cam = std::get_if<Camera*>(&child)) {
            (*cam)->set_viewport(v);
        }
        if (c.layout == HORIZONTAL) {
            v.x += v.width + padding * 2;
            if (leftover == 0) v.width--;
        } else {
            v.y += v.height + padding * 2;
            if (leftover == 0) v.height--;
        }
        leftover--;
    }
}
void ScreenPartition::draw_delimiters(){
    ImGui::SetNextWindowBgAlpha(0.f);
    bool open = false;
    ImGui::Begin("Lines", &open, ImGuiWindowFlags_NoTitleBar |ImGuiWindowFlags_NoResize |ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
    auto dl = ImGui::GetWindowDrawList();
    dl->PushClipRectFullScreen();
    auto& v = viewport;
    //dl->AddRect({(float)v.x,(float)v.y},{(float)v.x+v.width, (float)v.y+v.height}, ImColor(100,100,100));
    for (auto& cam : all_cam) {
        const auto& v = cam.viewport();
        dl->AddRect({(float)v.x-1,(float)v.y-1},{(float)v.x+v.width+1, (float)v.y+v.height+1}, ImColor(100,100,100));
    }
    dl->PopClipRect();
    ImGui::End();
}
