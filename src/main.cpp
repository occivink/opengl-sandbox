#include "engine.hpp"
#include "cube.hpp"
#include "textured_quad.hpp"
#include "scancodes.hpp"
#include "camera.hpp"
#include "volume.hpp"
#include "manipulator.hpp"
#include "utils.hpp"
#include "log.hpp"

#include "imgui/imgui.h"
#include "emscripten.h"

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
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO

#include "stb/stb_image_write.h"

using namespace std::string_literals;

namespace {
    bool log_window = true;

    bool painting_mode = false;
    float label_opacity = 0.5f;
    float label_radius = 20.f;
    int label_color = 1;

    const std::vector<const char*> label_sizes = { "128", "256", "512", "1024", "2048", "4096" };
    int label_width = 0;  // just an index, the real value is (128 * (1 << index))
    int label_height = 0; // same

    std::unique_ptr<Manipulator> manip;
    std::unique_ptr<Cube> cube;
    std::unique_ptr<Volume> volume;
    std::unique_ptr<TexturedQuad> labels;
    std::unique_ptr<TexturedQuad> quad;

    std::vector<unsigned char> current_image_data;

    ScreenPartition part;
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
            cube.reset();
            volume.reset();
            manip.reset();
            current_image_data.resize(n);
            memcpy(current_image_data.data(), data, n);
        }
    }
}

void loop_func()
{
    auto& in = Engine::input();

    //manip->handle_input(part.all_cam[0].projection_view(), in);
    if (in.sizeChanged) {
        part.set_viewport({0,0,in.width,in.height});
    }

    part.draw_delimiters();

    for (auto& cam : part.all_cam) {
        cam.handle_input(in);

        const auto& v = cam.viewport();
        glViewport(v.x, v.y, v.width, v.height);
        for (auto& other_cam : part.all_cam)
            if (&cam != &other_cam)
                other_cam.render(cam);
        if (manip)
            manip->render(cam.projection_view());
        else if (volume)
            volume->render(cam);
        else if (cube)
            cube->render(cam, glm::vec3(1,1,1));
        else if (quad and labels)
            quad->renderWithLabels(cam, *labels, label_opacity);
        else if (quad)
            quad->render(cam);
    }

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

        if (ImGui::Button("Manip")) {
            if (manip)
                manip->set_mode(manip->mode() == Manipulator::Rotation ? Manipulator::Translation : Manipulator::Rotation);
            else
                manip.reset(new Manipulator);
        }
        if (ImGui::Button("Cube")) {
            cube.reset(new Cube);
            volume.reset();
            quad.reset();
            manip.reset();
        }
        if (ImGui::Button("Volume")) {
            manip.reset();
            cube.reset();
            quad.reset();
            volume.reset(new Volume({64,64,64}, 4));
        }

        /*if (ImGui::Button("Request random image")) {
            fetchRandomImage();
        }
        if (ImGui::Button("Send Image") && quad) {
            sendSerializedImage("SendCurrentImage", current_image_data);
        }
        if  (ImGui::Button("Send Labels") && labels) {
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
        }*/

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
            for (size_t i = 0; i < label_sizes.size(); i++) {
                int val = 128 * (1 << i);
                if (ImGui::Selectable(label_sizes[i], val == label_width))
                    label_width = i;
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::BeginCombo("size", label_sizes[label_height])) {
            for (size_t i = 0; i < label_sizes.size(); i++) {
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

    part.all_cam[0].draw_widget();

    //ImGui::ShowDemoWindow();
}

int main()
{
    stbi_set_flip_vertically_on_load(true);

    Engine::init(loop_func);

    part.horizontal();
    for (auto& cam : part.all_cam)
        cam.set_position({0,0,5});
    resetLabels();

    Engine::start();
    Engine::fini();
}
