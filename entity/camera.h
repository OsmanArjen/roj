#pragma once
#include <cmath>

#include <glm/glm.hpp>

#include <app/input.h>

namespace roj {
    // =========================================================================
    // [3] camera                                          >>> FILE: roj/camera.h
    // =========================================================================

    // What is seen: pose + projection. Nothing about how it is driven.
    struct Camera {
        glm::vec3 position{ 0.0f, 0.0f, -5.0f };
        float yaw = 0.0f;      // degrees
        float pitch = 0.0f;      // degrees
        float fovY = 60.0f;     // degrees
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
    };

    // One way of driving a Camera. An orbit / game / scripted controller would be
    // a sibling struct writing to the same Camera.
    struct FlyController {
        float moveSpeed = 9.0f;
        float sprintScale = 3.0f;
        float lookSpeed = 0.12f;    // degrees per pixel of mouse travel
        float wheelStep = 1.15f;    // speed multiplier per wheel notch (multiplicative
        // so it feels even at any speed)
        float minSpeed = 0.5f;
        float maxSpeed = 500.0f;
    };

    inline glm::vec3 cameraForward(const Camera* c) {
        const float yr = glm::radians(c->yaw);
        const float pr = glm::radians(c->pitch);
        return glm::normalize(glm::vec3(
            std::sin(yr) * std::cos(pr),
            std::sin(pr),
            std::cos(yr) * std::cos(pr)
        ));
    }

    inline glm::vec3 cameraRight(const Camera* c) {
        return glm::normalize(glm::cross(glm::vec3(0, 1, 0), cameraForward(c)));
    }

    inline glm::vec3 cameraUp(const Camera* c) {
        return glm::normalize(glm::cross(cameraForward(c), cameraRight(c)));
    }

    inline void cameraRotate(Camera* c, float yawDeltaDeg, float pitchDeltaDeg) {
        c->yaw = std::fmod(c->yaw + yawDeltaDeg, 360.0f);                // keep float precision bounded
        c->pitch = glm::clamp(c->pitch + pitchDeltaDeg, -89.0f, 89.0f);
    }

    // Mouse-look + WASD fly. Needs a captured mouse, otherwise the cursor is
    // doing something else (UI, picking) and the camera must stay put.
    void flyControllerUpdate(FlyController* fc, Camera* cam, const Input& in, float dt);

    // Write 16 floats (column-major, bgfx layout) into out16.
    void cameraViewMatrix(const Camera* c, float* out16);
    void cameraProjMatrix(const Camera* c, float aspect, bool homogeneousDepth, float* out16);

}