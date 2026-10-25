#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entt/core/fwd.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>

namespace roj {


    template <typename T>
    struct AnimationTrack {
        std::vector<float> times;
        std::vector<T> values;
    };


    struct AnimationChannel {
        entt::id_type node = 0;     
        AnimationTrack<glm::vec3> position;
        AnimationTrack<glm::quat> rotation;
        AnimationTrack<glm::vec3> scale;
    };

    struct AnimationClip {
        std::string name;
        float duration = 0.0f;      
        std::vector<AnimationChannel> channels;
    };



    struct NodePose { glm::vec3 position; glm::quat rotation; glm::vec3 scale; };
    struct RigJoint { entt::id_type node; glm::mat4 inverseBind; };


    struct Rig {
        std::vector<entt::entity> nodes;    // node index  -> entity
        std::vector<NodePose> rest;         // node index  -> local pose at instantiation
        std::vector<RigJoint> joints;       // joint index -> node + inverse bind matrix
    };

    
    struct AnimationPlayer {
        entt::entity clip = entt::null;
        float time = 0.0f;
        float speed = 1.0f;
        bool loop = true;
        bool playing = false;
    };

    struct AnimationPlayDesc {
        bool  loop = true;
        float speed = 1.0f;   
        float phase = 0.0f;   
    };

    struct SkinPalette {
        entt::entity rig = entt::null;      
        std::vector<glm::mat4> matrices;    
    };


    Rig makeRig(const entt::registry& reg, std::vector<entt::entity> nodes, std::vector<RigJoint> joints);


    entt::entity animationFindClip(const entt::registry& reg, std::string_view name);
    void animationPlay(entt::registry& reg, entt::entity rigEntity, entt::entity clip, bool loop = true);
    bool animationPlay(entt::registry& reg, entt::entity rigEntity, entt::entity clip, const AnimationPlayDesc& desc);
    bool animationPlay(entt::registry& reg, entt::entity rigEntity, std::string_view clipName, const AnimationPlayDesc& desc = {});
    bool animationEnsure(entt::registry& reg, entt::entity rigEntity, entt::entity clip, const AnimationPlayDesc& desc = {});
    void animationStop(entt::registry& reg, entt::entity rigEntity);

    void animationPause(entt::registry& reg, entt::entity rigEntity);
    void animationResume(entt::registry& reg, entt::entity rigEntity);
    void animationSetSpeed(entt::registry& reg, entt::entity rigEntity, float speed);
    void animationSeek(entt::registry& reg, entt::entity rigEntity, float phase);

    entt::entity animationCurrent(const entt::registry& reg, entt::entity rigEntity);
    bool  animationIsPlaying(const entt::registry& reg, entt::entity rigEntity);
    bool  animationFinished(const entt::registry& reg, entt::entity rigEntity);
    float animationPhase(const entt::registry& reg, entt::entity rigEntity);

    void animationUpdate(entt::registry& reg, float dt);
    void skinUpdate(entt::registry& reg);
}