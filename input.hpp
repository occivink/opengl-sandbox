#pragma once

#include <mat4x4.hpp>
#include <climits>
#include <array>

struct Input {
    Input()
    {
        width = -1;
        height = -1;
        sizeChanged = false;

        mousePos = { INT_MAX, INT_MAX };
        mouseDelta = { INT_MAX, INT_MAX };
        mouseDown.fill(false);
        mouseStateChanged.fill(false);
        mouseWheel = 0;

        keyCtrl = false;
        keyAlt = false;
        keyShift = false;
        keyDown.fill(false);
        keyStateChanged.fill(false);

        mouseCaptured = false;
        keyboardCaptured = false;
    }
    Input(const Input&) = default;
    Input& operator=(const Input&) = default;

    void setChangedFlags(const Input& previous)
    {
        for (int i = 0; i < 512; ++i)
            keyStateChanged[i] = (keyDown[i] != previous.keyDown[i]);
        for (int i = 0; i < 5; ++i)
            mouseStateChanged[i] = (mouseDown[i] != previous.mouseDown[i]);
        mouseDelta = mousePos - previous.mousePos;
        sizeChanged = (width != previous.width) or (height != previous.height);
    }

    int width, height;
    bool sizeChanged;

    glm::ivec2 mousePos;
    glm::ivec2 mouseDelta;
    std::array<bool,5> mouseDown;
    std::array<bool,5> mouseStateChanged;
    int mouseWheel; // positive is up

    bool keyCtrl;
    bool keyAlt;
    bool keyShift;
    std::array<bool,512> keyDown;
    std::array<bool,512> keyStateChanged;

    bool mouseCaptured;
    bool keyboardCaptured;
};
