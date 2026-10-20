#include <app/platform.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

#include <SDL3/SDL.h>

#include <app/log.h>

namespace roj {

    namespace {

        void nativeWindowHandles(SDL_Window* window, void** outWindow, void** outDisplay) {
            const SDL_PropertiesID props = SDL_GetWindowProperties(window);
            *outWindow = nullptr;
            *outDisplay = nullptr;
#if defined(_WIN32)
            *outWindow = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(__APPLE__)
            *outWindow = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#elif defined(__linux__)
            const char* driver = SDL_GetCurrentVideoDriver();
            if (driver && std::strcmp(driver, "x11") == 0) {
                *outDisplay = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
                *outWindow = (void*)(uintptr_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
            }
            else if (driver && std::strcmp(driver, "wayland") == 0) {
                *outDisplay = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
                *outWindow = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
            }
#else
            (void)props;
#endif
        }

        ActionMask actionsForKey(const Platform& p, SDL_Scancode key) {
            ActionMask actions = 0;
            for (uint32_t i = 0; i < p.bindingCount; ++i)
                if (p.bindings[i].key == key)
                    actions |= p.bindings[i].action;
            return actions;
        }

        uint32_t mouseButtonBit(uint8_t sdlButton) {
            return (sdlButton >= 1 && sdlButton <= 5) ? (1u << (sdlButton - 1)) : 0u;
        }

        void applyMouseMode(const Platform& p) {
            SDL_SetWindowRelativeMouseMode(p.window, p.mouseCaptured && p.focused);
        }

    } 

    bool platformBind(Platform& p, SDL_Scancode key, ActionMask action) {
        if (p.bindingCount >= kMaxBindings) {
            logWrite(LogError, "Binding table full (max %u)", (unsigned)kMaxBindings);
            return false;
        }
        p.bindings[p.bindingCount++] = KeyBinding{ key, action };
        return true;
    }

    void platformUnbindAction(Platform& p, ActionMask action) {
        uint32_t out = 0;
        for (uint32_t i = 0; i < p.bindingCount; ++i) {
            KeyBinding b = p.bindings[i];
            b.action &= ~action;
            if (b.action != 0)
                p.bindings[out++] = b;
        }
        p.bindingCount = out;
    }

    void platformSetMouseCaptured(Platform& p, bool captured) {
        p.mouseCaptured = captured;
        applyMouseMode(p);
    }

    void platformToggleFullscreen(Platform& p) {
        const bool fullscreen = (SDL_GetWindowFlags(p.window) & SDL_WINDOW_FULLSCREEN) != 0;
        SDL_SetWindowFullscreen(p.window, !fullscreen);   // size change arrives as an event
    }

    void platformSetTitle(Platform& p, const char* title) {
        SDL_SetWindowTitle(p.window, title);
    }

    void platformSleep(uint32_t ms) {
        SDL_Delay(ms);
    }

    void platformShutdown(Platform& p) {
        if (p.window)
            SDL_DestroyWindow(p.window);
        p = Platform{};
        SDL_Quit();
    }

    bool platformInit(Platform& p, const PlatformInit& init) {
        p = Platform{};

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            logWrite(LogError, "SDL_Init failed: %s", SDL_GetError());
            return false;
        }

        p.window = SDL_CreateWindow(init.title, init.width, init.height, init.flags);
        if (!p.window) {
            logWrite(LogError, "SDL_CreateWindow failed: %s", SDL_GetError());
            SDL_Quit();
            return false;
        }
        p.windowId = SDL_GetWindowID(p.window);

        nativeWindowHandles(p.window, &p.nativeWindow, &p.nativeDisplay);
        if (!p.nativeWindow) {
            logWrite(LogError, "No native window handle for this platform / video driver (see nativeWindowHandles)");
            platformShutdown(p);
            return false;
        }

        bool bound = true;
        if (init.useDefaultBindings)
            for (const KeyBinding& b : kDefaultBindings)
                bound = platformBind(p, b.key, b.action) && bound;
        if (init.extraBindings)
            for (uint32_t i = 0; i < init.extraBindingCount; ++i)
                bound = platformBind(p, init.extraBindings[i].key, init.extraBindings[i].action) && bound;
        if (!bound) {
            platformShutdown(p);
            return false;
        }

        int pw = 0, ph = 0;
        SDL_GetWindowSizeInPixels(p.window, &pw, &ph);
        p.pixelSize = { pw, ph };
        platformSetMouseCaptured(p, init.mouseCaptured);
        return true;
    }

    void platformPollEvents(Platform& p, Input& in, void* event_callback) {
        in = Input{};
        p.resized = false;

        bool sizeDirty = false;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ((bool (*)(const SDL_Event*))event_callback)(&ev);

            switch (ev.type) {
            case SDL_EVENT_QUIT:
                in.quit = true;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (ev.window.windowID == p.windowId)
                    in.quit = true;
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                if (ev.window.windowID == p.windowId)
                    sizeDirty = true;
                break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                if (ev.window.windowID == p.windowId) {
                    p.focused = true;
                    applyMouseMode(p);
                }
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if (ev.window.windowID == p.windowId) {
                    p.focused = false;
                    applyMouseMode(p);
                }
                break;
            case SDL_EVENT_KEY_DOWN:
                // Edges come from the event stream so a press + release inside one
                // frame is never lost (polling key state alone would miss it).
                if (ev.key.windowID == p.windowId && !ev.key.repeat)
                    in.pressed |= actionsForKey(p, ev.key.scancode);
                break;
            case SDL_EVENT_KEY_UP:
                if (ev.key.windowID == p.windowId)
                    in.released |= actionsForKey(p, ev.key.scancode);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (ev.motion.windowID == p.windowId && p.mouseCaptured && p.focused)
                    in.lookDelta += glm::vec2(ev.motion.xrel, ev.motion.yrel);
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                if (ev.wheel.windowID == p.windowId)
                    in.wheel += ev.wheel.y;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (ev.button.windowID == p.windowId)
                    in.mousePressed |= mouseButtonBit(ev.button.button);
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (ev.button.windowID == p.windowId)
                    in.mouseReleased |= mouseButtonBit(ev.button.button);
                break;
            default:
                break;
            }
        }

        // Platform-owned actions are resolved here; the rest are passed upward.
        if (in.pressed & ActQuit)
            in.quit = true;
        if (in.pressed & ActToggleMouse)
            platformSetMouseCaptured(p, !p.mouseCaptured);
        if (in.pressed & ActToggleFullscreen)
            platformToggleFullscreen(p);

        if (sizeDirty) {
            int pw = 0, ph = 0;
            SDL_GetWindowSizeInPixels(p.window, &pw, &ph);
            const glm::ivec2 size{ pw, ph };
            if (size != p.pixelSize) {
                p.pixelSize = size;
                p.resized = true;
            }
        }

        p.minimized = (SDL_GetWindowFlags(p.window) & SDL_WINDOW_MINIMIZED) != 0
            || p.pixelSize.x <= 0 || p.pixelSize.y <= 0;

        // Level state: scan the table so two keys bound to one action behave.
        // Nothing is "held" in an unfocused window.
        if (p.focused) {
            const bool* keys = SDL_GetKeyboardState(nullptr);
            for (uint32_t i = 0; i < p.bindingCount; ++i)
                if (keys[p.bindings[i].key])
                    in.held |= p.bindings[i].action;
        }
        in.released &= ~in.held;   // another bound key still holds it

        float mx = 0.0f, my = 0.0f;
        in.mouseHeld = SDL_GetMouseState(&mx, &my);
        const float density = SDL_GetWindowPixelDensity(p.window);
        const float scale = density > 0.0f ? density : 1.0f;
        in.mousePos = glm::vec2(mx * scale, my * scale);

        in.mouseCaptured = p.mouseCaptured && p.focused;
        in.focused = p.focused;
    }

}