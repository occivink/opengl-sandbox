#include "engine.hpp"
#include "cube.hpp"
#include "textured_quad.hpp"
#include "scancodes.hpp"
#include "camera.hpp"
#include "log.hpp"

#include "imgui/imgui.h"
#include "emscripten.h"
#include "emscripten/fetch.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <functional>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


using namespace std::string_literals;

namespace {
    Camera cam;

    bool painting_mode = false;
    float label_opacity = 0.5f;
    float label_radius = 20.f;
    glm::vec3 label_color = { 0.2f, 0.4f, 0.6f };

    static const std::vector<const char*> label_sizes = { "128", "256", "512", "1024", "2048", "4096" }; 
    int label_width = 2;  // just an index, the real value is (128 * (1 << index))
    int label_height = 2; // just an index, the real value is (128 * (1 << index))

    std::optional<glm::vec2> previous_paint_location;

    std::unique_ptr<TexturedQuad> quad;
    std::unique_ptr<TexturedQuad> labels;

    #define PI 3.1415f

    float theta = 0.f;
    float phi = PI / 2;
    float distance = 1.f;
}


template<typename T>
class OnScopeEnd
{
public:
    [[gnu::always_inline]]
    OnScopeEnd(T func) : m_func(std::move(func)) {}

    [[gnu::always_inline]]
    ~OnScopeEnd() { m_func(); }
private:
    T m_func;
};

template<typename T>
OnScopeEnd<T> on_scope_end(T t)
{
    return OnScopeEnd<T>{std::move(t)};
}

void destroy_fetch(emscripten_fetch_t* f) { emscripten_fetch_close(f); }
std::unique_ptr<emscripten_fetch_t, decltype(&destroy_fetch)> last_image_req(nullptr, destroy_fetch);

void downloadSucceeded(emscripten_fetch_t *fetch)
{
    Log::Info("download succeeded");
    if (fetch->numBytes > 0) {
        last_image_req.reset(fetch);
        Log::Info("replacing current image");
        int w, h, channels;
        unsigned char* mem = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(fetch->data), fetch->numBytes, &w, &h, &channels, 4);
        if (!mem)
            return;
        quad.reset(new TexturedQuad(mem, w, h, 4));
        stbi_image_free(mem);
    } else {
        destroy_fetch(fetch);
    }
}

void downloadFailed(emscripten_fetch_t *fetch)
{
    Log::Info("download failed");
    emscripten_fetch_close(fetch);
}

void randomImage()
{
    Log::Info("fetching random image");
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    auto path = std::string("api/RandomImage");
    emscripten_fetch(&attr, path.c_str());
}

void processCurrentImage()
{
    if (!last_image_req)
        return;
    Log::Info("processing current image");
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "POST");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    int n = last_image_req->numBytes;
    Log::Info("sending " + std::to_string(n) + " bytes");
    auto bur = new char[n];
    std::memcpy(bur, last_image_req->data, n);
    attr.requestDataSize = n;
    attr.requestData = bur;
    auto path = std::string("api/ProcessImage");
    emscripten_fetch(&attr, path.c_str());
}

extern "C" {

    void loadImage() {
        auto fd = open("/file.txt", O_RDONLY);
        auto close_fd = on_scope_end([&]() { close(fd); });
        if (fd == -1) {
            Log::Info("can't open");
            return;
        }

        struct stat st {};
        fstat(fd, &st);
        if (S_ISDIR(st.st_mode)) {
            Log::Info("is a dir");
            return;
        }
        if (st.st_size == 0) {
            Log::Info("empty");
            return;
        }

        auto data = (const unsigned char*)mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) {
            Log::Info("mmap failed");
            return;
        }
        auto mun = on_scope_end([&]() { munmap((void*)data, st.st_size); });

        int w, h, channels;
        unsigned char* mem = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(data), st.st_size, &w, &h, &channels, 4);
        if (!mem) {
            Log::Info("not a png");
            return;
        }
        quad.reset(new TexturedQuad(mem, w, h, 4));
        stbi_image_free(mem);
    }

}

void refreshCamera() {
    glm::vec3 pos = {
        distance * sin(phi) * sin(theta),
        distance * cos(phi),
        distance * sin(phi) * cos(theta),
    };
    auto right = glm::cross(-pos, glm::vec3(0,1,0));
    cam.look_at(
        pos,
        glm::vec3(0, 0, 0),
        glm::cross(right, -pos)
    );
    cam.set_perspective(70.0f, (float)Engine::width() / (float)Engine::height(), 0.1f, 100.0f);
}

void loop_func()
{
    auto& m_io = ImGui::GetIO();
    bool paint = false;
    bool rotate_camera = false;
    bool zoom_camera = false;
    if (!m_io.WantCaptureMouse) {
        if (m_io.MouseDown[0]) {
            if (painting_mode)
                paint = true;
            else 
                rotate_camera = (m_io.MouseDelta.x != 0.f or m_io.MouseDelta.y != 0.f);
        }
        zoom_camera = m_io.MouseWheel != 0.f;
    }
    if (rotate_camera) {
        auto& delta = m_io.MouseDelta;
        theta -= delta.x / 500.f;
        phi -= delta.y / 500.f;
        if (phi < 0.001)
            phi = 0.001;
        if (phi > PI - 0.001)
            phi = PI - 0.001;
    }
    if (zoom_camera) {
        distance += m_io.MouseWheel * -0.1f;
        distance = std::max(0.01f, distance);
    }
    if (rotate_camera or zoom_camera) {
        Log::Trace("refreshing camera");
        refreshCamera();
    }
    if (painting_mode) {
        if (paint and quad and labels) {
            glm::vec2 cursor = {
                2.0f * m_io.MousePos[0] / (float)Engine::width() - 1.0f,
                1.0f - 2.0f * m_io.MousePos[1] / (float)Engine::height(),
            };
            glm::vec2 outPicked;
            if (quad->unproject(cam, nullptr, cursor, outPicked)) {
                float radius_adjusted = label_radius * labels->height() / quad->height();
                if (previous_paint_location)
                    labels->paint(*previous_paint_location, outPicked, label_color, radius_adjusted, quad->ratio());
                else
                    labels->paint(outPicked, outPicked, label_color, radius_adjusted, quad->ratio());
                previous_paint_location = outPicked;
            }
        } else {
            previous_paint_location.reset();
        }
    }

    if (quad and labels)
        quad->renderWithLabels(cam, *labels, label_opacity);
    else if (quad)
        quad->render(cam);

    if (!Engine::show_gui())
        return;

	ImVec2 pos;
    static bool log_window = true;
    {
        ImGui::Begin("Main");

        // see engine.hpp for why this thing need special handling
        ImGui::Button("Open image...");
        Engine::setOpenHovered(ImGui::IsItemHovered());
        if (ImGui::Button("Request random image"))
            randomImage();
        if (ImGui::Button("Process"))
            processCurrentImage();
        ImGui::Checkbox("Paint", &painting_mode);
        ImGui::Checkbox("Log window", &log_window);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);


        pos = ImGui::GetWindowPos();
        pos.x += ImGui::GetWindowWidth() + 10;
        ImGui::End();
    }

    if (painting_mode) {
        ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver);
        ImGui::Begin("Paint", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PushItemWidth(ImGui::GetWindowWidth() / 3.0f);

        if (ImGui::BeginCombo("x", label_sizes[label_width])) {
            for (int i = 0; i < label_sizes.size(); i++) {
                int val = 128 * (1 << i);
                if (ImGui::Selectable(label_sizes[i], val == label_width))
                    label_width = i;
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::BeginCombo("size", label_sizes[label_height])) {
            for (int i = 0; i < label_sizes.size(); i++) {
                int val = 128 * (1 << i);
                if (ImGui::Selectable(label_sizes[i], val == label_height))
                    label_height = i;
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::SliderFloat("Opacity", &label_opacity, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Radius", &label_radius, 1.0f, 100.0f, "%.2f");
        ImGui::ColorEdit3("Color", &label_color[0]);
        if (ImGui::Button("Clear"))
            labels.reset(new TexturedQuad(128 * (1 << label_width), 128 * (1 << label_height), 4, true));

        ImGui::End();
    }

    if (log_window)
        Log::draw_widget();
}

int main(int argc, char** argv)
{
    randomImage();
    Engine::init(loop_func);
    labels.reset(new TexturedQuad(128 * (1 << label_width), 128 * (1 << label_height), 4, true));
    refreshCamera();
    Engine::start();
    Engine::fini();
}
