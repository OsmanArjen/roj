#pragma once
#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <app/input.h>
#include <entity/camera.h>

namespace roj {
    // =========================================================================
    // third-person camera                      >>> FILE: roj/third_person_camera.h
    // =========================================================================
    //
    // A sibling of FlyController: it drives the same Camera (position / yaw /
    // pitch), so cameraViewMatrix(), cameraProjMatrix() and cameraRotation()
    // work unchanged. Same conventions as camera.h (left-handed, +Z forward,
    // yaw > 0 turns right, pitch > 0 looks up).
    //
    // Per frame, in this order:
    //   1. move the character (see thirdPersonMoveDirection / thirdPersonFacingYaw)
    //   2. thirdPersonUpdate(&tps, &camera, characterPos, input, dt)
    //   3. build view / projection from the camera as usual
    // Updating the camera BEFORE the character moves makes it trail one frame.

    // Optional world query so the camera doesn't sit inside walls. Cast a ray
    // from `from` (the pivot) toward `to` (the wanted camera position) and
    // return the distance from `from` to the first obstruction, or any value
    // >= |to - from| when the way is clear.
    using CameraProbeFn = float (*)(void* user, const glm::vec3& from, const glm::vec3& to);

    struct ThirdPersonController {
        // --- framing -------------------------------------------------------
        float pivotHeight = 1.6f;   // pivot above the target's origin (assumes origin at the feet)
        float shoulderOffset = 0.0f;   // > 0 puts the character on the left of the screen (over-the-right-shoulder)
        float distance = 4.0f;   // desired distance behind the pivot
        float minDistance = 1.0f;
        float maxDistance = 10.0f;
        float zoomStep = 1.15f;  // distance multiplier per wheel notch

        // --- look ----------------------------------------------------------
        float lookSpeed = 0.12f;  // degrees per pixel of mouse travel
        float minPitch = -70.0f;  // looking down (camera above the character)
        float maxPitch = 35.0f;  // looking up (camera drops below the pivot)

        // --- smoothing (1/seconds; larger = tighter, <= 0 = rigid) ---------
        float followSharpness = 12.0f;  // pivot chasing the character
        float zoomSharpness = 10.0f;  // wheel zoom
        float recoverSharpness = 4.0f;   // easing back out after a collision pull-in

        // --- collision -----------------------------------------------------
        float groundClearance = 0.25f;  // camera stays this far above the target's origin height
        CameraProbeFn probe = nullptr;
        void* probeUser = nullptr;
        float probeMargin = 0.25f;  // stay this far in front of a hit

        // --- state (written by thirdPersonUpdate) --------------------------
        glm::vec3 smoothPivot{ 0.0f };
        float zoomDistance = 4.0f;
        float limitDistance = 4.0f;
        bool snapped = false;
    };

    // Jump the follow state to `targetPos` with no easing (spawn, teleport,
    // scene change). thirdPersonUpdate calls this itself on the first frame.
    inline void thirdPersonSnap(ThirdPersonController* tc, const glm::vec3& targetPos) {
        tc->smoothPivot = targetPos;
        tc->zoomDistance = tc->limitDistance = glm::clamp(tc->distance, tc->minDistance, tc->maxDistance);
        tc->snapped = true;
    }

    // Frame-rate independent exponential smoothing factor.
    inline float cameraDampFactor(float sharpness, float dt) {
        return sharpness > 0.0f ? 1.0f - std::exp(-sharpness * dt) : 1.0f;
    }

    // Yaw-only basis: movement stays on the ground plane however far the
    // camera is pitched.
    inline glm::vec3 cameraPlanarForward(const Camera* c) {
        const float y = glm::radians(c->yaw);
        return { std::sin(y), 0.0f, std::cos(y) };
    }
    inline glm::vec3 cameraPlanarRight(const Camera* c) {
        const float y = glm::radians(c->yaw);
        return { std::cos(y), 0.0f, -std::sin(y) };
    }

    inline void thirdPersonUpdate(ThirdPersonController* tc, Camera* cam,
        const glm::vec3& targetPos, const Input& in, float dt) {
        if (!tc->snapped)
            thirdPersonSnap(tc, targetPos);

        // Look and zoom only while the mouse is captured; following the
        // character always continues.
        if (in.mouseCaptured) {
            cameraRotate(cam, in.lookDelta.x * tc->lookSpeed, -in.lookDelta.y * tc->lookSpeed);
            if (in.wheel != 0.0f)
                tc->distance = glm::clamp(tc->distance * std::pow(tc->zoomStep, -in.wheel),
                    tc->minDistance, tc->maxDistance);
        }
        cam->pitch = glm::clamp(cam->pitch, tc->minPitch, tc->maxPitch);

        tc->smoothPivot += (targetPos - tc->smoothPivot) * cameraDampFactor(tc->followSharpness, dt);
        const glm::vec3 pivot = tc->smoothPivot + glm::vec3(0.0f, tc->pivotHeight, 0.0f);

        const glm::quat rot = cameraRotation(cam);
        const glm::vec3 fwd = rot * glm::vec3(0.0f, 0.0f, 1.0f);
        const glm::vec3 right = rot * glm::vec3(1.0f, 0.0f, 0.0f);

        // How far back the camera may go this frame, from the world's point of view.
        float allowed = tc->distance;

        // Ground: back off along the line through the pivot instead of
        // clamping the height, so the character stays centred in view.
        if (fwd.y > 1e-4f) {
            const float floorY = tc->smoothPivot.y + tc->groundClearance;
            allowed = std::min(allowed, std::max((pivot.y - floorY) / fwd.y, 0.5f));
        }

        // Walls: ask the world.
        if (tc->probe) {
            const glm::vec3 wanted = pivot + right * tc->shoulderOffset - fwd * tc->distance;
            const float len = glm::length(wanted - pivot);
            const float hit = tc->probe(tc->probeUser, pivot, wanted);
            if (len > 1e-4f && hit < len)
                allowed = std::min(allowed, tc->distance * std::max(hit - tc->probeMargin, 0.0f) / len);
        }

        // Pull in instantly (never show the inside of a wall), ease back out.
        if (allowed < tc->limitDistance)
            tc->limitDistance = allowed;
        else
            tc->limitDistance += (allowed - tc->limitDistance) * cameraDampFactor(tc->recoverSharpness, dt);

        tc->zoomDistance += (tc->distance - tc->zoomDistance) * cameraDampFactor(tc->zoomSharpness, dt);

        const float effective = std::min(tc->zoomDistance, tc->limitDistance);
        const float pull = tc->zoomDistance > 1e-4f ? glm::clamp(effective / tc->zoomDistance, 0.0f, 1.0f) : 1.0f;

        cam->position = pivot + right * (tc->shoulderOffset * pull) - fwd * effective;
    }

    // Camera-relative ground-plane move direction from the standard move
    // actions: unit length, or zero when there is no input (or the mouse is not
    // captured, so typing into UI doesn't walk the character).
    inline glm::vec3 thirdPersonMoveDirection(const Camera* c, const Input& in) {
        if (!in.mouseCaptured)
            return glm::vec3(0.0f);

        const glm::vec3 f = cameraPlanarForward(c);
        const glm::vec3 r = cameraPlanarRight(c);

        glm::vec3 d(0.0f);
        if (in.held & ActMoveForward) d += f;
        if (in.held & ActMoveBack)    d -= f;
        if (in.held & ActMoveRight)   d += r;
        if (in.held & ActMoveLeft)    d -= r;

        const float l2 = glm::dot(d, d);
        return l2 > 1e-8f ? d * (1.0f / std::sqrt(l2)) : glm::vec3(0.0f);
    }

    // Turn the character toward its direction of travel along the shortest arc.
    // Angles are radians about +Y. Returns the new facing yaw; unchanged when
    // moveDir is zero. modelYawOffset corrects for the model's own forward axis
    // (0 if it faces +Z, glm::pi<float>() if it faces -Z).
    inline float thirdPersonFacingYaw(float currentYaw, const glm::vec3& moveDir,
        float turnSpeedRadPerSec, float dt, float modelYawOffset = 0.0f) {
        if (glm::dot(moveDir, moveDir) < 1e-8f)
            return currentYaw;

        const float target = std::atan2(moveDir.x, moveDir.z) + modelYawOffset;
        const float diff = std::remainder(target - currentYaw, glm::two_pi<float>());   // (-pi, pi]
        const float maxStep = turnSpeedRadPerSec * dt;
        return std::remainder(currentYaw + glm::clamp(diff, -maxStep, maxStep), glm::two_pi<float>());
    }

}