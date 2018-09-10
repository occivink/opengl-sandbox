#pragma once

#include <functional>
#include <array>

#include "input.hpp"

namespace Engine {
    void init(std::function<void()> loop_func);
    void start();
    void fini();

    bool& show_gui();
    bool& quit();
    std::array<float,4>& clear_color();
    bool visible();
    double elapsed_time();

    Input& input();

    void setOpenHovered(bool v);
};
