#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace roj {

    using ActionMask = uint64_t;

    enum Action : ActionMask {
        ActNone = 0,
        ActQuit = ActionMask(1) << 0,    // Esc
        ActToggleMouse = ActionMask(1) << 1,    // Tab
        ActToggleHud = ActionMask(1) << 2,    // F1
        ActToggleStats = ActionMask(1) << 3,    // F2
        ActToggleWireframe = ActionMask(1) << 4,    // F3
        ActToggleVsync = ActionMask(1) << 5,    // F4
        ActCycleMsaa = ActionMask(1) << 6,    // F5
        ActToggleFullscreen = ActionMask(1) << 7,    // F11
        ActMoveForward = ActionMask(1) << 8,    // W
        ActMoveBack = ActionMask(1) << 9,    // S
        ActMoveLeft = ActionMask(1) << 10,   // A
        ActMoveRight = ActionMask(1) << 11,   // D
        ActMoveUp = ActionMask(1) << 12,   // Space
        ActMoveDown = ActionMask(1) << 13,   // Left Ctrl
        ActMoveFast = ActionMask(1) << 14,   // Left Shift
        ActTogglePause = ActionMask(1) << 15,   // P
        ActUser = ActionMask(1) << 16,   // first app-defined bit
    };

    enum MouseButton : uint32_t {
        MouseLeft = 1u << 0,
        MouseMiddle = 1u << 1,
        MouseRight = 1u << 2,
        MouseX1 = 1u << 3,
        MouseX2 = 1u << 4,
    };


    struct Input {
        ActionMask pressed = 0;
        ActionMask released = 0;
        ActionMask held = 0;

        glm::vec2  lookDelta{ 0.0f };   
        float      wheel = 0.0f;

        glm::vec2  mousePos{ 0.0f }; 
        uint32_t   mouseHeld = 0;
        uint32_t   mousePressed = 0;
        uint32_t   mouseReleased = 0;

        bool       mouseCaptured = false;
        bool       focused = true;
        bool       quit = false;
    };

    inline bool inputPressed(const Input* in, ActionMask mask) { return (in->pressed & mask) != 0; }
    inline bool inputReleased(const Input* in, ActionMask mask) { return (in->released & mask) != 0; }
    inline bool inputHeld(const Input* in, ActionMask mask) { return (in->held & mask) != 0; }


    inline float inputAxis(const Input* in, ActionMask negative, ActionMask positive) {
        return (inputHeld(in, positive) ? 1.0f : 0.0f) - (inputHeld(in, negative) ? 1.0f : 0.0f);
    }

    inline glm::vec3 inputMoveAxes(const Input* in) {
        return glm::vec3(inputAxis(in, ActMoveLeft, ActMoveRight),
            inputAxis(in, ActMoveDown, ActMoveUp),
            inputAxis(in, ActMoveBack, ActMoveForward));
    }

}