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
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include "stb_image_write.h"

using namespace std::string_literals;

namespace {
    Camera cam;

    bool log_window = true;

    bool painting_mode = false;
    float label_opacity = 0.5f;
    float label_radius = 20.f;
    int label_color = 1;

    const std::vector<const char*> label_sizes = { "128", "256", "512", "1024", "2048", "4096" };
    int label_width = 0;  // just an index, the real value is (128 * (1 << index))
    int label_height = 0; // just an index, the real value is (128 * (1 << index))

    std::optional<glm::vec2> previous_paint_location;

    std::unique_ptr<TexturedQuad> quad;
    std::unique_ptr<TexturedQuad> labels;

    std::vector<unsigned char> current_image_data;

    #define PI 3.1415f

    float theta = 0.f;
    float phi = PI / 2;
    float distance = 2.f;
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

void stbi_write_callback(void *context, void *data, int size) {
    auto& png = *reinterpret_cast<std::vector<unsigned char>*>(context);
    auto old_size = png.size();
    png.resize(old_size + size);
    std::memcpy(&png[old_size], data, size);
}
bool pixels_to_png(const std::vector<unsigned char>& pixels, int width, int height, int channels, std::vector<unsigned char>& png) {
    int res = stbi_write_png_to_func(stbi_write_callback, &png, width, height, channels, pixels.data(), width * channels);
    return res != 0;
}

void resetLabels() {
    labels.reset(new TexturedQuad(128 * (1 << label_width), 128 * (1 << label_height), 3, true));
}

bool loadImageToQuad(const unsigned char* image_data, int size)
{
    int w, h, channels;
    unsigned char* mem = stbi_load_from_memory(image_data, size, &w, &h, &channels, 4);
    if (!mem) {
        Log::Error("failed to load image");
        return false;
    }
    quad.reset(new TexturedQuad(mem, w, h, 4));
    stbi_image_free(mem);
    return true;
}

void fetchImageSuccess(emscripten_fetch_t *fetch)
{
    auto c = on_scope_end([&]{ emscripten_fetch_close(fetch); });
    if (fetch->numBytes == 0) {
        Log::Error("expected data, but received none");
        return;
    }
    Log::Info("trying to load received data (" + std::to_string(fetch->numBytes/1000) + "kb) as image");
    if (loadImageToQuad(reinterpret_cast<const unsigned char*>(fetch->data), fetch->numBytes)) {
        current_image_data.resize(fetch->numBytes);
        std::memcpy(current_image_data.data(), fetch->data, current_image_data.size());
    }
}

void getProcessingResultSuccess(emscripten_fetch_t *fetch)
{
    auto c = on_scope_end([&]{ emscripten_fetch_close(fetch); });
    if (fetch->numBytes == 0) {
        Log::Error("expected data, but received none");
        return;
    }
    Log::Info("trying to load received data (" + std::to_string(fetch->numBytes/1000) + "kb) as image");
    if (loadImageToQuad(reinterpret_cast<const unsigned char*>(fetch->data), fetch->numBytes)) {
        current_image_data.resize(fetch->numBytes);
        std::memcpy(current_image_data.data(), fetch->data, current_image_data.size());
        resetLabels();
    }
}

void genericSuccess(emscripten_fetch_t *fetch)
{
    Log::Info("api call succeeded");
    emscripten_fetch_close(fetch);
}

void genericFail(emscripten_fetch_t *fetch)
{
    Log::Error("api call failed");
    emscripten_fetch_close(fetch);
}

void sendSerializedImage(const char* apiPath, const std::vector<unsigned char>& png_data)
{
    Log::Info("sending image to "s + apiPath);
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "POST");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = genericSuccess;
    attr.onerror = genericFail;
    int n = png_data.size();
    auto tmp = new char[n];
    std::memcpy(tmp, png_data.data(), n);
    attr.requestDataSize = n;
    attr.requestData = tmp;
    auto s = "api/"s + apiPath;
    emscripten_fetch(&attr, s.c_str());
}

void getProcessingResult()
{
    Log::Info("waiting for processing result");
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = getProcessingResultSuccess;
    attr.onerror = genericFail;
    emscripten_fetch(&attr, "api/ProcessImages");
}

void fetchRandomImage()
{
    Log::Info("fetching random image");
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = fetchImageSuccess;
    attr.onerror = genericFail;
    emscripten_fetch(&attr, "api/RandomImage");
}

extern "C" { // necessary to export to js
    void loadImageFile() {
        auto fd = open("/file.txt", O_RDONLY);
        auto close_fd = on_scope_end([&]() { close(fd); });
        if (fd == -1) {
            Log::Info("can't open file");
            return;
        }
        struct stat st;
        fstat(fd, &st);
        if (S_ISDIR(st.st_mode)) {
            Log::Info("file is a dir?");
            return;
        }
        if (st.st_size == 0) {
            Log::Info("file empty?");
            return;
        }

        auto data = (const unsigned char*)mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) {
            Log::Info("mmap failed");
            return;
        }
        auto c = on_scope_end([&]{ munmap((void*)data, st.st_size); });
        int n = st.st_size;
        if (loadImageToQuad(data, n)) {
            current_image_data.resize(n);
            memcpy(current_image_data.data(), data, n);
        }
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
                    labels->paint(outPicked, *previous_paint_location, label_color, radius_adjusted, quad->ratio());
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
    {
        ImGui::SetNextWindowPos({10,10}, ImGuiCond_FirstUseEver);
        ImGui::Begin("Main");

        // see engine.hpp for why this thing need special handling
        ImGui::Button("Open image...");
        Engine::setOpenHovered(ImGui::IsItemHovered());
        ImGui::Checkbox("Paint", &painting_mode);

        if (ImGui::Button("Request random image")) {
            fetchRandomImage();
        }
        if (ImGui::Button("Send Image") && quad) {
            sendSerializedImage("SendCurrentImage", current_image_data);
        }
        if (ImGui::Button("Send Labels") && labels) {
            std::vector<unsigned char> pixels;
            if (labels->exportPixels(pixels)) {
                int culled_size = pixels.size() / 3;
                for (int i = 0; i < culled_size; ++i)
                    pixels[i] = pixels[i * 3];
                pixels.resize(culled_size);
                std::vector<unsigned char> asPng;
                if (pixels_to_png(pixels, labels->width(), labels->height(), 1, asPng)) {
                    pixels.clear();
                    sendSerializedImage("SendLabelImage", asPng);
                }
            }
        }
        if (ImGui::Button("Process")) {
            getProcessingResult();
        }

        ImGui::Checkbox("Log window", &log_window);
        ImGui::Text("%.2f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

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
        ImGui::SliderInt("Color", &label_color, 0, 2);
        if (ImGui::Button("Clear"))
            resetLabels();

        ImGui::End();
    }

    if (log_window)
        Log::draw_widget();
}

int main(int argc, char** argv)
{
    fetchRandomImage();
    Engine::init(loop_func);
    resetLabels();
    refreshCamera();
    Engine::start();
    Engine::fini();
}
