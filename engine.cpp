#include "engine.hpp"

#include "imgui/imgui.h"
#include "imgui_opengles_impl.hpp"
#include "scancodes.hpp"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>

namespace Engine {
namespace {
    ImGuiIO* m_io;
    std::function<void()> m_loop_func;

    bool m_show_gui = true;
    bool m_quit = false;
    std::array<float,4> m_clear_color = {0.f, 0.f, 0.f, 1.f};
    double m_last_time;
    double m_elapsed_time = 1/60.f;
    bool m_visible = true;

    EM_BOOL visibilityCallback(int eventType, const EmscriptenVisibilityChangeEvent *visEvent, void *userData)
    {
        m_visible = !visEvent->hidden;
        return false;
    }

    EM_BOOL canvasSizeCallback(int eventType, const void* reserved, void* userData) {
        int w, h;
        emscripten_get_canvas_element_size("#canvas", &w, &h);
        m_io->DisplaySize = ImVec2((float)w, (float)h);
        m_io->DisplayFramebufferScale = ImVec2(1, 1);
        return true;
    }

    EM_BOOL keyUpDownCallback(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
        if (keyEvent->keyCode >= sizeof(ScanCode::keyCodeToScanCode))
            return false;
        int scancode = ScanCode::keyCodeToScanCode[keyEvent->keyCode];
        if (scancode == ScanCode::S_UNKNOWN)
            return false;
        IM_ASSERT(scancode >= 0 && scancode < IM_ARRAYSIZE(m_io->KeysDown));
        m_io->KeysDown[scancode] = (eventType == EMSCRIPTEN_EVENT_KEYDOWN);
        m_io->KeyShift = keyEvent->shiftKey;
        m_io->KeyCtrl =  keyEvent->ctrlKey;
        m_io->KeyAlt =   keyEvent->altKey;
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

    EM_BOOL mouseMoveCallback(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
        m_io->MousePos = { (float)mouseEvent->canvasX, (float)mouseEvent->canvasY };
        return true;
    }

    EM_BOOL mouseWheelCallback(int eventType, const EmscriptenWheelEvent *mouseEvent, void *userData) {
        if (mouseEvent->deltaX < 0) m_io->MouseWheelH += 1;
        if (mouseEvent->deltaX > 0) m_io->MouseWheelH -= 1;
        if (mouseEvent->deltaY < 0) m_io->MouseWheel += 1;
        if (mouseEvent->deltaY > 0) m_io->MouseWheel -= 1;
        return true;
    }

    EM_BOOL mouseClickCallback(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
        m_io->MouseDown[mouseEvent->button] = (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN);
        m_io->KeyShift = mouseEvent->shiftKey;
        m_io->KeyCtrl  = mouseEvent->ctrlKey;
        m_io->KeyAlt   = mouseEvent->altKey;
        m_io->KeySuper = mouseEvent->metaKey;
        return true;
    }

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE createContext() {
        EmscriptenWebGLContextAttributes attrs;
        emscripten_webgl_init_context_attributes(&attrs);
        attrs.majorVersion = 2;
        attrs.minorVersion = 0;
        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context(0, &attrs);
        return context;
    }

    void makeCurrent(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context) {
        EMSCRIPTEN_RESULT res = emscripten_webgl_make_context_current(context);
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
        if (m_show_gui) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui::NewFrame();
        }
        glViewport(0, 0, (int)m_io->DisplaySize.x, (int)m_io->DisplaySize.y);
        glClearColor(m_clear_color[0], m_clear_color[1], m_clear_color[2], m_clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT);

        m_loop_func();

        if (m_show_gui) {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        m_last_time = current_time;
    }
}

void init(std::function<void()> func)
{
    m_loop_func = std::move(func);

    auto glContext = createContext();
    makeCurrent(glContext);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_io = &ImGui::GetIO();

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
    //io.Fonts->AddFontDefault();
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

    EmscriptenFullscreenStrategy fsStrategy = { };
    fsStrategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_DEFAULT;
    fsStrategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF;
    fsStrategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_BILINEAR;
    fsStrategy.canvasResizedCallback = canvasSizeCallback;
    fsStrategy.canvasResizedCallbackUserData = nullptr;
    emscripten_enter_soft_fullscreen(nullptr, &fsStrategy);

    emscripten_set_keydown_callback(0, nullptr, true, keyUpDownCallback);
    emscripten_set_keyup_callback(0, nullptr, true, keyUpDownCallback);
    emscripten_set_keypress_callback(0, nullptr, true, keyPressCallback);
    emscripten_set_mousemove_callback(0, nullptr, true, mouseMoveCallback);
    emscripten_set_mouseup_callback(0, nullptr, true, mouseClickCallback);
    emscripten_set_wheel_callback(0, nullptr, true, mouseWheelCallback);
    emscripten_set_mousedown_callback(0, nullptr, true, mouseClickCallback);

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

}
