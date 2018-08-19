#include "log.hpp"
#include "engine.hpp"
#include "time.h"
#include "imgui/imgui.h"

#include <vector>

namespace Log {

namespace {
    struct Line {
        uint64_t timestamp;
        enum Type {
            Fatal  = 1 << 0, // fatal messages only
            Error  = 1 << 1, // error messages
            Warn   = 1 << 2, // warning messages
            Info   = 1 << 3, // informational messages
            Status = 1 << 4, // status messages (default)
            Debug  = 1 << 5, // debug messages
            Trace  = 1 << 6, // very noisy debug messages
        } type;
        std::string content;
    };

    bool m_wasAtEnd = true;
    bool m_grew = false;
    uint8_t m_filter = 0b00011111;

    std::vector<Line> m_log; // TODO: replace with ringbuffer
    std::vector<int> m_filtered;

    uint64_t getTimeAsMs() {
        struct timespec spec;
        clock_gettime(CLOCK_MONOTONIC, &spec);
        return spec.tv_sec * 1000 + spec.tv_nsec / 1.0e6;
    }

    void Log(Line::Type t, std::string s) {
        if (t & m_filter) {
            m_filtered.push_back(m_log.size());
            m_grew = true;
        }
        m_log.push_back(Line{ getTimeAsMs(), t, std::move(s) });
    }
}

void Fatal(std::string s)  { Log(Line::Fatal,  std::move(s)); }
void Error(std::string s)  { Log(Line::Error,  std::move(s)); }
void Warn(std::string s)   { Log(Line::Warn,   std::move(s)); }
void Info(std::string s)   { Log(Line::Info,   std::move(s)); }
void Status(std::string s) { Log(Line::Status, std::move(s)); }
void Debug(std::string s)  { Log(Line::Debug,  std::move(s)); }
void Trace(std::string s)  { Log(Line::Trace,  std::move(s)); }

void draw_widget() {
    ImGui::SetNextWindowPos(ImVec2((float)Engine::width() / 2, (float)Engine::height() - 150.f), ImGuiCond_FirstUseEver, ImVec2(0.5, 0.5f));
    ImGui::SetNextWindowSize(ImVec2((float)Engine::width() * 3.f / 4.f, 250), ImGuiCond_FirstUseEver);
    ImGui::Begin("Log window");
    if (ImGui::Button("Clear")) {
        m_log.clear();
        m_filtered.clear();
    }
    ImGui::SameLine();
    if (ImGui::BeginCombo("", "Filters"))
    {
        static const std::vector<const char*> filter_names = { "Fatal", "Error", "Warn", "Info", "Status", "Debug", "Trace" };
        bool modified = false;
        for (int i = 0; i < 7; i++) {
            bool selected = m_filter & (1 << i);
            if (ImGui::Selectable(filter_names[i], selected, ImGuiSelectableFlags_DontClosePopups)) {
                modified = true;
                if (selected)
                    m_filter &= ~(1 << i);
                else
                    m_filter |= (1 << i);
            }
        }
        ImGui::EndCombo();
        if (modified) {
            m_filtered.clear();
            for (int i = 0; i < m_log.size(); ++i)
                if (m_filter & m_log[i].type)
                    m_filtered.push_back(i);
        }
    }
    ImGui::SameLine();
    static bool showMs = false;
    ImGui::Checkbox("Show ms", &showMs);

    ImGui::BeginChild("Log", {0,0}, false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

        ImGuiListClipper clipper(m_filtered.size());
        bool show_hours = !m_log.empty() and m_log.back().timestamp > (60 * 60 * 1000);
        std::vector<int> range;
        while (clipper.Step()) {
            range.push_back(clipper.DisplayStart);
            range.push_back(clipper.DisplayEnd);
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                const auto& line = m_log[m_filtered[i]];
                std::string s;
                s.reserve(12);
                auto add_num = [&](int i, int pad_to = 2) {
                    int cop = i;
                    while (i /= 10) pad_to--;
                    for (int i = 1; i < pad_to; ++i)
                        s += '0';
                    s += std::to_string(cop);
                };
                if (show_hours) {
                    add_num(line.timestamp / (60 * 60 * 1000));
                    s += ":";
                }
                add_num((line.timestamp / (60 * 1000)) % 60);
                s += ":";
                add_num((line.timestamp / 1000) % 60);
                if (showMs) {
                    s += ".";
                    add_num(line.timestamp % 1000, 3);
                }
                ImGui::Text("%s | %s", s.c_str(), line.content.c_str());
            }
        }

        if (m_grew and m_wasAtEnd)
            ImGui::SetScrollHere();
        else
            m_wasAtEnd = (ImGui::GetScrollY() + 0.0001f >= ImGui::GetScrollMaxY());
        m_grew = false;

        ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::End();
}

}
