#ifndef ANIMATOR_HPP
#define ANIMATOR_HPP
#include "skinned_model.hpp"
namespace roj
{
class Animator
{
private:
    std::vector<glm::mat4> m_boneMatrices;
    Animation* m_currAnim{nullptr};
    SkinnedModel* m_model;
    float m_currTime{0.0f};
private:
    int getKeyTransformIdx(float animTime, std::vector<float>& timestamps);
    float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
    glm::mat4 interpolatePosition(float animTime, BoneTransform& boneTransform);
    glm::mat4 interpolateRotation(float animTime, BoneTransform& boneTransform);
    glm::mat4 interpolateScaling(float animTime, BoneTransform& boneTransform);
    void calcBoneTransform(BoneNode& node, glm::mat4 offset);
public:
	Animator(SkinnedModel& model);
    void set(const std::string& name);
    std::vector<std::string> get();
    void update(float dt);
};
}
#endif //-ANIMATOR_HPP
