#pragma once
#include <cstdint>

#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>

#include <glm/glm.hpp>

#include <app/input.h>

#include <entt/signal/delegate.hpp>
namespace roj {

    struct KeyBinding {
        SDL_Scancode key;
        ActionMask   action;
    };

    inline constexpr KeyBinding kDefaultBindings[] = {
        { SDL_SCANCODE_ESCAPE, ActQuit },
        { SDL_SCANCODE_TAB,    ActToggleMouse },
        { SDL_SCANCODE_F1,     ActToggleHud },
        { SDL_SCANCODE_F2,     ActToggleStats },
        { SDL_SCANCODE_F3,     ActToggleWireframe },
        { SDL_SCANCODE_F4,     ActToggleVsync },
        { SDL_SCANCODE_F5,     ActCycleMsaa },
        { SDL_SCANCODE_F11,    ActToggleFullscreen },
        { SDL_SCANCODE_P,      ActTogglePause },
        { SDL_SCANCODE_W,      ActMoveForward },
        { SDL_SCANCODE_S,      ActMoveBack },
        { SDL_SCANCODE_A,      ActMoveLeft },
        { SDL_SCANCODE_D,      ActMoveRight },
        { SDL_SCANCODE_SPACE,  ActMoveUp },
        { SDL_SCANCODE_LCTRL,  ActMoveDown },
        { SDL_SCANCODE_LSHIFT, ActMoveFast },
    };

    inline constexpr uint32_t kMaxBindings = 128;

    struct PlatformInit {
        const char* title = "roj";
        int32_t width = 1280;
        int32_t height = 720;
        SDL_WindowFlags   flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        bool mouseCaptured = true;
        bool useDefaultBindings = true;
        const KeyBinding* extraBindings = nullptr;   // copied; need not outlive init
        uint32_t extraBindingCount = 0;
    };

    struct Platform {
        SDL_Window* window = nullptr;
        SDL_WindowID windowId = 0;
        void* nativeWindow = nullptr;
        void* nativeDisplay = nullptr;   // X11 / Wayland only; null on Windows / macOS
        glm::ivec2   pixelSize{ 0 };           // drawable size in pixels (HiDPI aware)
        bool         mouseCaptured = false;    // what the app WANTS; applied only while focused
        bool         resized = false;       // pixelSize changed during the last poll
        bool         minimized = false;       // minimized, or a zero-sized drawable
        bool         focused = true;

        KeyBinding   bindings[kMaxBindings] = {};  // owned copy: no caller-lifetime coupling
        uint32_t     bindingCount = 0;
    };

    struct Frame {
        float    dt = 0.0f;
        float    realDt = 0.0f;
        double   time = 0.0;
        uint64_t index = 0;
        bool     minimized = false;
        roj::Input    input;
    };

    bool platformInit(Platform& p, const PlatformInit& init);
    void platformShutdown(Platform& p);
    void platformPollEvents(Platform& p, Input& in, void* event_callback);

    bool platformBind(Platform& p, SDL_Scancode key, ActionMask action);
    void platformUnbindAction(Platform& p, ActionMask action);

    void platformSetMouseCaptured(Platform& p, bool captured);
    void platformToggleFullscreen(Platform& p);
    void platformSetTitle(Platform& p, const char* title);
    void platformSleep(uint32_t ms);

}