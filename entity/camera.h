#pragma once
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <app/input.h>

namespace roj {
    // =========================================================================
    // [3] camera                                          >>> FILE: roj/camera.h
    // =========================================================================
    //
    // Conventions (match bx::mtxLookAt / bx::mtxProj, i.e. left-handed):
    //   Y is up, +X is right, +Z is forward at yaw = pitch = 0.
    //   yaw   > 0 turns right (toward +X)
    //   pitch > 0 looks up
    //
    // cameraRotation() is the ONE place these conventions are encoded. Basis
    // vectors, the view matrix and anything attached to the camera (the fps
    // viewmodel) are all derived from it, so they can never disagree.

    // What is seen: pose + projection. Nothing about how it is driven.
    struct Camera {
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
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

    // Camera orientation as a quaternion: yaw about +Y, then pitch about the
    // camera's own X axis.
    //
    // NOTE the minus on pitch: angleAxis follows the right-hand rule, so a
    // positive angle about +X rotates +Z *downward*. The camera defines
    // positive pitch as "look up", hence the negation.
    inline glm::quat cameraRotation(const Camera* c) {
        const glm::quat qYaw = glm::angleAxis(glm::radians(c->yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::quat qPitch = glm::angleAxis(glm::radians(-c->pitch), glm::vec3(1.0f, 0.0f, 0.0f));
        return qYaw * qPitch;
    }

    inline glm::vec3 cameraForward(const Camera* c) { return cameraRotation(c) * glm::vec3(0.0f, 0.0f, 1.0f); }
    inline glm::vec3 cameraRight(const Camera* c) { return cameraRotation(c) * glm::vec3(1.0f, 0.0f, 0.0f); }
    inline glm::vec3 cameraUp(const Camera* c) { return cameraRotation(c) * glm::vec3(0.0f, 1.0f, 0.0f); }

    // Camera-space point -> world space. For things that ride on the camera:
    //   t.position = cameraLocalToWorld(cam, { 0.25f, -0.2f, 0.5f });  // +Z = forward
    //   t.rotation = cameraRotation(cam) * modelDefaultRotation;
    // Run it AFTER the camera was updated this frame, or the model lags one frame.
    inline glm::vec3 cameraLocalToWorld(const Camera* c, const glm::vec3& local) {
        return c->position + cameraRotation(c) * local;
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