#include "engine.hpp"
#include "input.hpp"
#include "log.hpp"
#include "scancodes.hpp"

#include "imgui/imgui.h"
#include "imgui_opengles_impl.hpp"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>

namespace Engine {
namespace {
    ImGuiIO* m_io;
    std::function<void()> m_loop_func;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE m_glContext = 0;
    int m_width = 0;
    int m_height = 0;
    bool m_show_gui = true;
    bool m_quit = false;
    bool m_start_fullscreen = true;
    std::array<float,4> m_clear_color = {0.2f, 0.4f, 0.6f, 1.f};
    double m_last_time;
    double m_elapsed_time = 1/60.f;
    bool m_visible = true;

    Input m_prev_input;
    Input m_input;

    bool m_released_touch = false;
    bool m_openHovered = false;

    EM_BOOL focusInOutCallback(int eventType, const EmscriptenFocusEvent *focusEvent, void *userData)
    {
        for (int i = 0; i < 512; ++i)
            m_input.keyDown[i] = m_io->KeysDown[i] = false;
        for (int i = 0; i < 5; ++i)
            m_input.mouseDown[i] = m_io->MouseDown[i] = false;
        m_io->MousePos = { -FLT_MAX, -FLT_MAX };
        m_input.keyShift = m_io->KeyShift = false;
        m_input.keyCtrl  = m_io->KeyCtrl  = false;
        m_input.keyAlt   = m_io->KeyAlt   = false;
        m_io->KeySuper = false;

        return false;
    }

    EM_BOOL canvasSizeCallback(int eventType, const void* reserved, void* userData) {
        emscripten_get_canvas_element_size("#canvas", &m_width, &m_height);
        m_input.width = m_width;
        m_input.height = m_height;
        m_io->DisplaySize = ImVec2((float)m_width, (float)m_height);
        m_io->DisplayFramebufferScale = ImVec2(1, 1);
        return true;
    }

    EM_BOOL keyUpDownCallback(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
        if (keyEvent->keyCode >= sizeof(ScanCode::keyCodeToScanCode))
            return false;
        int scancode = ScanCode::keyCodeToScanCode[keyEvent->keyCode];
        if (scancode == ScanCode::S_UNKNOWN)
            return false;

        // hack: browsers only allow an input of type file to be triggered in user event callbacks
        if (keyEvent->ctrlKey and scancode == ScanCode::S_O and eventType == EMSCRIPTEN_EVENT_KEYDOWN) {
            m_io->KeyCtrl =  false;
            Log::Info("Opening file...");
            emscripten_run_script("document.getElementById('fileElem').click();");
            return true;
        }


        IM_ASSERT(scancode >= 0 and scancode < IM_ARRAYSIZE(m_io->KeysDown));
        m_input.keyDown[scancode] = m_io->KeysDown[scancode] = (eventType == EMSCRIPTEN_EVENT_KEYDOWN);
        m_input.keyShift = m_io->KeyShift = keyEvent->shiftKey;
        m_input.keyCtrl  = m_io->KeyCtrl =  keyEvent->ctrlKey;
        m_input.keyAlt   = m_io->KeyAlt =   keyEvent->altKey;
        m_io->KeySuper = keyEvent->metaKey;

        return false;
    }

    EM_BOOL keyPressCallback(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
        char text[5];
        unsigned long codepoint = keyEvent->charCode;
        if (codepoint <= 0x7F) {
            text[0] = (char) codepoint;
            text[1] = '\0';
        } else if (codepoint <= 0x7FF) {
            text[0] = 0xC0 | (char) ((codepoint >> 6) & 0x1F);
            text[1] = 0x80 | (char) (codepoint & 0x3F);
            text[2] = '\0';
        } else if (codepoint <= 0xFFFF) {
            text[0] = 0xE0 | (char) ((codepoint >> 12) & 0x0F);
            text[1] = 0x80 | (char) ((codepoint >> 6) & 0x3F);
            text[2] = 0x80 | (char) (codepoint & 0x3F);
            text[3] = '\0';
        } else if (codepoint <= 0x10FFFF) {
            text[0] = 0xF0 | (char) ((codepoint >> 18) & 0x0F);
            text[1] = 0x80 | (char) ((codepoint >> 12) & 0x3F);
            text[2] = 0x80 | (char) ((codepoint >> 6) & 0x3F);
            text[3] = 0x80 | (char) (codepoint & 0x3F);
            text[4] = '\0';
        } else
            return false;
        m_io->AddInputCharactersUTF8(text);
        return false;
    }

    EM_BOOL touchMoveCallback(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData) {
        if (touchEvent->numTouches == 0)
            return true;
        m_input.mousePos = { touchEvent->touches[0].canvasX, touchEvent->touches[0].canvasY };
        m_io->MousePos = { (float)m_input.mousePos.x, (float)m_input.mousePos.y };
        return true;
    }
    EM_BOOL touchStartEndCallback(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData) {
        if (touchEvent->numTouches == 0)
            return true;
        if (m_openHovered and eventType != EMSCRIPTEN_EVENT_TOUCHSTART) {
            Log::Info("Opening file...");
            emscripten_run_script("document.getElementById('fileElem').click();");
            // we don't get the corresponding mouse up so might as well abort
            return false;
        }
        m_input.mouseDown[0] = m_io->MouseDown[0] = (eventType == EMSCRIPTEN_EVENT_TOUCHSTART);
        m_input.mousePos = { touchEvent->touches[0].canvasX, touchEvent->touches[0].canvasY };
        m_io->MousePos = { (float)m_input.mousePos.x, (float)m_input.mousePos.y };
        if (eventType != EMSCRIPTEN_EVENT_TOUCHSTART)
            m_released_touch = true;
        return true;
    }


    EM_BOOL mouseMoveCallback(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
        m_input.mousePos = { mouseEvent->canvasX, mouseEvent->canvasY };
        m_io->MousePos = { (float)m_input.mousePos.x, (float)m_input.mousePos.y };
        return true;
    }
    EM_BOOL mouseClickCallback(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
        if (m_openHovered and mouseEvent->button == 0 and eventType == EMSCRIPTEN_EVENT_MOUSEDOWN) {
            Log::Info("Opening file...");
            emscripten_run_script("document.getElementById('fileElem').click();");
            // we don't get the corresponding mouse up so might as well abort
            return false;
        }

		int i = mouseEvent->button;
        m_input.mouseDown[i] = m_io->MouseDown[i] = (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN);
        m_input.keyShift = m_io->KeyShift = mouseEvent->shiftKey;
        m_input.keyCtrl  = m_io->KeyCtrl =  mouseEvent->ctrlKey;
        m_input.keyAlt   = m_io->KeyAlt =   mouseEvent->altKey;
        m_io->KeySuper = mouseEvent->metaKey;
        m_input.mousePos = { mouseEvent->canvasX, mouseEvent->canvasY };
        m_io->MousePos = { (float)m_input.mousePos.x, (float)m_input.mousePos.y };
        return true;
    }
    EM_BOOL mouseWheelCallback(int eventType, const EmscriptenWheelEvent *mouseEvent, void *userData) {
        if (mouseEvent->deltaY < 0) { m_io->MouseWheel += 1; m_input.mouseWheel++; }
        if (mouseEvent->deltaY > 0) { m_io->MouseWheel -= 1; m_input.mouseWheel--; }
        if (mouseEvent->deltaX < 0) { m_io->MouseWheelH += 1; }
        if (mouseEvent->deltaX > 0) { m_io->MouseWheelH -= 1; }
        return true;
    }

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE createContext() {
        EmscriptenWebGLContextAttributes attrs;
        emscripten_webgl_init_context_attributes(&attrs);
        attrs.enableExtensionsByDefault = 1;
        attrs.majorVersion = 2;
        attrs.minorVersion = 0;
        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context(0, &attrs);
        return context;
    }

    void makeCurrent(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context) {
        emscripten_webgl_make_context_current(context);
    }

    void mainloop()
    {
        if (m_quit) {
            emscripten_cancel_main_loop();
            return;
        }
        double current_time = emscripten_get_now() / 1000;
        m_elapsed_time = current_time - m_last_time;
        m_io->DeltaTime = m_elapsed_time;
        m_input.setChangedFlags(m_prev_input);
        m_input.mouseCaptured = m_io->WantCaptureMouse; 
        m_input.keyboardCaptured = m_io->WantCaptureKeyboard;

        if (m_show_gui) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui::NewFrame();
        }

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glViewport(0, 0, m_width, m_height);
        glClearColor(m_clear_color[0], m_clear_color[1], m_clear_color[2], m_clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_loop_func();

        if (m_show_gui) {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        if (m_released_touch)
        {
            m_io->MousePos = { -FLT_MAX, -FLT_MAX };
            m_released_touch = false;
        }
        m_last_time = current_time;
        m_prev_input = m_input;
        m_input.mouseWheel = 0;
    }
}

void init(std::function<void()> func)
{
    m_loop_func = std::move(func);

    m_glContext = createContext();
    makeCurrent(m_glContext);


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_io = &ImGui::GetIO();

    double scale = emscripten_get_device_pixel_ratio();
    if (scale > 1.5f) {
        m_io->FontGlobalScale = scale;
    }
    ImGui::GetStyle().ScaleAllSizes(scale);

    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //m_io->Fonts->AddFont(fontConfig);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    m_io->KeyMap[ImGuiKey_Tab] = ScanCode::S_TAB;
    m_io->KeyMap[ImGuiKey_LeftArrow] = ScanCode::S_LEFT;
    m_io->KeyMap[ImGuiKey_RightArrow] = ScanCode::S_RIGHT;
    m_io->KeyMap[ImGuiKey_UpArrow] = ScanCode::S_UP;
    m_io->KeyMap[ImGuiKey_DownArrow] = ScanCode::S_DOWN;
    m_io->KeyMap[ImGuiKey_PageUp] = ScanCode::S_PAGEUP;
    m_io->KeyMap[ImGuiKey_PageDown] = ScanCode::S_PAGEDOWN;
    m_io->KeyMap[ImGuiKey_Home] = ScanCode::S_HOME;
    m_io->KeyMap[ImGuiKey_End] = ScanCode::S_END;
    m_io->KeyMap[ImGuiKey_Insert] = ScanCode::S_INSERT;
    m_io->KeyMap[ImGuiKey_Delete] = ScanCode::S_DELETE;
    m_io->KeyMap[ImGuiKey_Backspace] = ScanCode::S_BACKSPACE;
    m_io->KeyMap[ImGuiKey_Space] = ScanCode::S_SPACE;
    m_io->KeyMap[ImGuiKey_Enter] = ScanCode::S_RETURN;
    m_io->KeyMap[ImGuiKey_Escape] = ScanCode::S_ESCAPE;
    m_io->KeyMap[ImGuiKey_A] = ScanCode::S_A;
    m_io->KeyMap[ImGuiKey_C] = ScanCode::S_C;
    m_io->KeyMap[ImGuiKey_V] = ScanCode::S_V;
    m_io->KeyMap[ImGuiKey_X] = ScanCode::S_X;
    m_io->KeyMap[ImGuiKey_Y] = ScanCode::S_Y;
    m_io->KeyMap[ImGuiKey_Z] = ScanCode::S_Z;

    emscripten_get_canvas_element_size("#canvas", &m_width, &m_height);
    //int dontcare;
    //emscripten_get_canvas_size(&m_width, &m_height, &dontcare);
    m_io->DisplaySize = ImVec2((float)m_width, (float)m_height);
    m_io->DisplayFramebufferScale = ImVec2(1, 1);

    if (m_start_fullscreen) {
        EmscriptenFullscreenStrategy fsStrategy = { };
        fsStrategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_DEFAULT;
        fsStrategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF;
        fsStrategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_BILINEAR;
        fsStrategy.canvasResizedCallback = canvasSizeCallback;
        fsStrategy.canvasResizedCallbackUserData = nullptr;
        emscripten_enter_soft_fullscreen(nullptr, &fsStrategy);
    }

    emscripten_set_keydown_callback(0, nullptr, true, keyUpDownCallback);
    emscripten_set_keyup_callback(0, nullptr, true, keyUpDownCallback);
    emscripten_set_keypress_callback(0, nullptr, true, keyPressCallback);
    emscripten_set_touchmove_callback(0, nullptr, true, touchMoveCallback);
    emscripten_set_touchstart_callback(0, nullptr, true, touchStartEndCallback);
    emscripten_set_touchend_callback(0, nullptr, true, touchStartEndCallback);
    emscripten_set_mousemove_callback(0, nullptr, true, mouseMoveCallback);
    emscripten_set_mouseup_callback(0, nullptr, true, mouseClickCallback);
    emscripten_set_mousedown_callback(0, nullptr, true, mouseClickCallback);
    emscripten_set_wheel_callback(0, nullptr, true, mouseWheelCallback);
    emscripten_set_focusin_callback(0, nullptr, true, focusInOutCallback);
    emscripten_set_focusout_callback(0, nullptr, true, focusInOutCallback);
    emscripten_set_focus_callback(0, nullptr, true, focusInOutCallback);
}

void start() {
    emscripten_set_main_loop(mainloop, 0, 1);
}

void fini() {
    m_io = nullptr;
    emscripten_set_keydown_callback(0, nullptr, true, nullptr);
    emscripten_set_keyup_callback(0, nullptr, true, nullptr);
    emscripten_set_keypress_callback(0, nullptr, true, nullptr);
    emscripten_set_mousemove_callback(0, nullptr, true, nullptr);
    emscripten_set_mouseup_callback(0, nullptr, true, nullptr);
    emscripten_set_wheel_callback(0, nullptr, true, nullptr);
    emscripten_set_mousedown_callback(0, nullptr, true, nullptr);
    emscripten_set_focusin_callback(0, nullptr, true, nullptr);
    emscripten_set_focusout_callback(0, nullptr, true, nullptr);
    emscripten_set_focus_callback(0, nullptr, true, nullptr);

    emscripten_exit_soft_fullscreen();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}

bool& show_gui() {
    return m_show_gui;
}

bool& quit() {
    return m_quit;
}

std::array<float,4>& clear_color() {
    return m_clear_color;
}

bool visible(){
    return m_visible;
}

double elapsed_time(){
    return m_elapsed_time;
}

Input& input(){
    return m_input;
}

void setOpenHovered(bool v) {
    m_openHovered = v;
}
}
