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
    int getKeyTransformIdx(std::vector<float>& timestamps);
    float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
    glm::mat4 interpolatePosition(BoneTransform& boneTransform);
    glm::mat4 interpolateRotation(BoneTransform& boneTransform);
    glm::mat4 interpolateScaling(BoneTransform& boneTransform);
    void calcBoneTransform(BoneNode& node, glm::mat4 offset);
public:
	Animator(SkinnedModel& model);
    void set(const std::string& name);
    std::vector<std::string> get();
    std::vector<glm::mat4>& getBoneMatrices();
    void update(float dt);
    void reset() {

        std::cout << m_currTime << ':'<< m_currAnim->duration << '\n';        m_currTime = 0.0f;
    }
};
}
#endif //-ANIMATOR_HPP
