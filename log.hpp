#pragma once

#include <string>

namespace Log {
    void Fatal(std::string s);
    void Error(std::string s);
    void Warn(std::string s);
    void Info(std::string s);
    void Status(std::string s);
    void Debug(std::string s);
    void Trace(std::string s);

    void draw_widget();
}
