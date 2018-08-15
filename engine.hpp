#pragma once

#include <functional>
#include <array>

namespace Engine {
    void init(std::function<void()> loop_func);
    void fini();

    bool& show_gui();
    bool& quit();
    std::array<float,4>& clear_color();
    bool visible();
    double elapsed_time();
};
