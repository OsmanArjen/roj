#include <entity/camera.h>

#include <cmath>

#include <bx/math.h>

#include <glm/glm.hpp>

#include <app/input.h>

namespace roj {

    void flyControllerUpdate(FlyController* fc, Camera* cam,  const Input& in, float dt) {
        if (!in.mouseCaptured)
            return;

        cameraRotate(cam, in.lookDelta.x * fc->lookSpeed, -in.lookDelta.y * fc->lookSpeed);

        if (in.wheel != 0.0f)
            fc->moveSpeed = glm::clamp(fc->moveSpeed * std::pow(fc->wheelStep, in.wheel),
                fc->minSpeed, fc->maxSpeed);

        const glm::vec3 fwd = cameraForward(cam);
        const glm::vec3 rgt = cameraRight(cam);
        const glm::vec3 up(0.0f, 1.0f, 0.0f);

        glm::vec3 delta{ 0.0f };
        if (in.held & ActMoveForward) delta += fwd;
        if (in.held & ActMoveBack)    delta -= fwd;
        if (in.held & ActMoveRight)   delta += rgt;
        if (in.held & ActMoveLeft)    delta -= rgt;
        if (in.held & ActMoveUp)      delta += up;
        if (in.held & ActMoveDown)    delta -= up;

        if (glm::length(delta) > 0.0f) {
            const float speed = fc->moveSpeed * ((in.held & ActMoveFast) ? fc->sprintScale : 1.0f);
            cam->position += glm::normalize(delta) * speed * dt;
        }
    }

    void cameraViewMatrix(const Camera* c, float* out16) {
        const glm::vec3 eye = c->position;
        const glm::vec3 at = eye + cameraForward(c);
        const glm::vec3 up = cameraUp(c);
        bx::mtxLookAt(out16, { eye.x, eye.y, eye.z }, { at.x, at.y, at.z }, { up.x, up.y, up.z });
    }

    void cameraProjMatrix(const Camera* c, float aspect, bool homogeneousDepth, float* out16) {
        bx::mtxProj(out16, c->fovY, aspect, c->nearPlane, c->farPlane, homogeneousDepth);
    }

}