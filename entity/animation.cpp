#include <entity/animation.h>
#include <entity/transform.h>

#include <entt/entity/registry.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

namespace {


    struct KeySpan { size_t from, to; float blend; };

    KeySpan findSpan(const std::vector<float>& times, float time) {
        const size_t last = times.size() - 1;
        if (time <= times.front()) return { 0, 0, 0.0f };
        if (time >= times.back())  return { last, last, 0.0f };

        // First key strictly after `time`; the span is that key and the one before it.
        const size_t to = std::upper_bound(times.begin(), times.end(), time) - times.begin();
        const size_t from = to - 1;
        const float length = times[to] - times[from];
        return { from, to, length > 0.0f ? (time - times[from]) / length : 0.0f };
    }

    glm::vec3 interpolate(const glm::vec3& a, const glm::vec3& b, float t) { return glm::mix(a, b, t); }

    // Shortest-arc blend. glm::slerp falls back to a plain lerp for near-parallel inputs, so renormalise.
    glm::quat interpolate(const glm::quat& a, const glm::quat& b, float t) {
        return glm::normalize(glm::slerp(a, b, t));
    }

    // Leaves `out` untouched when the track is empty, so un-animated properties keep their value.
    template <typename T>
    void sampleTrack(const roj::AnimationTrack<T>& track, float time, T& out) {
        if (track.times.empty())
            return;
        const KeySpan span = findSpan(track.times, time);
        out = interpolate(track.values[span.from], track.values[span.to], span.blend);
    }

    // Brings `time` back into [0, duration]. Looping wraps; otherwise it clamps and stops playback.
    float wrapTime(float time, float duration, bool loop, bool& playing) {
        if (duration <= 0.0f)
            return 0.0f;
        if (loop) {
            time = std::fmod(time, duration);
            return time < 0.0f ? time + duration : time;
        }
        if (time < 0.0f || time > duration)
            playing = false;
        return std::clamp(time, 0.0f, duration);
    }



    void applyChannel(entt::registry& reg, const roj::Rig& rig, const roj::AnimationChannel& channel, float time) {
        if (channel.node >= rig.nodes.size())
            return;
        const entt::entity entity = rig.nodes[channel.node];
        if (!reg.valid(entity))
            return;

        reg.patch<roj::Transform>(entity, [&](roj::Transform& transform) {
            sampleTrack(channel.position, time, transform.position);
            sampleTrack(channel.rotation, time, transform.rotation);
            sampleTrack(channel.scale, time, transform.scale);
            });
    }

    // Node 0 is skipped: it carries the instance's placement in the world, which the caller owns.
    void restoreRestPose(entt::registry& reg, const roj::Rig& rig) {
        for (size_t i = 1; i < rig.nodes.size(); i++) {
            if (!reg.valid(rig.nodes[i]))
                continue;
            const roj::NodePose& rest = rig.rest[i];
            reg.patch<roj::Transform>(rig.nodes[i], [&](roj::Transform& transform) {
                transform.position = rest.position;
                transform.rotation = rest.rotation;
                transform.scale = rest.scale;
                });
        }
    }

    glm::mat4 globalMatrixOf(const entt::registry& reg, entt::entity entity) {
        const auto* global = reg.try_get<roj::Transform::GlobalMatrix>(entity);
        return global ? glm::mat4(*global) : glm::mat4(1.0f);
    }

} // namespace

namespace roj {

    Rig makeRig(const entt::registry& reg, std::vector<entt::entity> nodes, std::vector<RigJoint> joints) {
        Rig rig;
        rig.rest.reserve(nodes.size());
        for (const entt::entity entity : nodes) {
            const Transform& transform = reg.get<Transform>(entity);
            rig.rest.push_back({ transform.position, transform.rotation, transform.scale });
        }
        rig.nodes = std::move(nodes);
        rig.joints = std::move(joints);
        return rig;
    }

    void animationPlay(entt::registry& reg, entt::entity rigEntity, entt::entity clip, bool loop) {
        const Rig* rig = reg.try_get<Rig>(rigEntity);
        if (!rig)
            return;

        restoreRestPose(reg, *rig);

        AnimationPlayer& player = reg.get_or_emplace<AnimationPlayer>(rigEntity);
        player.clip = clip;
        player.time = 0.0f;
        player.loop = loop;
        player.playing = true;
    }

    void animationUpdate(entt::registry& reg, float dt) {
        reg.view<AnimationPlayer, Rig>().each([&](AnimationPlayer& player, const Rig& rig) {
            if (!player.playing || !reg.valid(player.clip))
                return;
            const AnimationClip* clip = reg.try_get<AnimationClip>(player.clip);
            if (!clip)
                return;

            player.time = wrapTime(player.time + dt * player.speed, clip->duration, player.loop, player.playing);
            for (const AnimationChannel& channel : clip->channels)
                applyChannel(reg, rig, channel, player.time);
            });
    }

    void skinUpdate(entt::registry& reg) {
        reg.view<SkinPalette>().each([&](entt::entity entity, SkinPalette& skin) {
            if (!reg.valid(skin.rig))
                return;
            const Rig* rig = reg.try_get<Rig>(skin.rig);
            if (!rig)
                return;

            // The renderer multiplies by the mesh's own world matrix afterwards, so the palette
            // must land in the mesh's local space: undo that matrix here.
            const glm::mat4 meshFromWorld = glm::inverse(globalMatrixOf(reg, entity));

            skin.matrices.resize(rig->joints.size());
            for (size_t j = 0; j < rig->joints.size(); j++) {
                const RigJoint& joint = rig->joints[j];
                const glm::mat4 jointWorld = globalMatrixOf(reg, rig->nodes[joint.node]);
                skin.matrices[j] = meshFromWorld * jointWorld * joint.inverseBind;
            }
            });
    }

} // namespace roj