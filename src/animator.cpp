#include "animator.hpp"



int roj::Animator::getKeyTransformIdx(std::vector<float>& timestamps)
{
    if (timestamps.size() < 2)
        return -1;
    
    for (int index = 0; index < timestamps.size() - 2; ++index)
    {
        if (m_currTime < timestamps[index + 1])
            return index;
    }
    
}

float roj::Animator::getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
    float scaleFactor = 0.0f;
    float midWayLength = m_currTime - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    scaleFactor = midWayLength / framesDiff;
    return scaleFactor;
}

glm::mat4 roj::Animator::interpolatePosition(roj::BoneTransform& boneTransform)
{
    int posIdx = getKeyTransformIdx(boneTransform.positionTimestamps);
    if (posIdx == -1)
        return glm::translate(glm::mat4(1.0f), boneTransform.positions[0]);

    float scaleFactor = getScaleFactor(boneTransform.positionTimestamps[posIdx],
        boneTransform.positionTimestamps[posIdx + 1], m_currTime);
    glm::vec3 finalPosition = glm::mix(boneTransform.positions[posIdx],
        boneTransform.positions[posIdx+1], scaleFactor);
    return glm::translate(glm::mat4(1.0f), finalPosition);

}

glm::mat4 roj::Animator::interpolateRotation(roj::BoneTransform& boneTransform)
{
    int posIdx = getKeyTransformIdx(boneTransform.rotationTimestamps);
    if (posIdx == -1)
        return glm::toMat4(glm::normalize(boneTransform.rotations[0]));

    
    float scaleFactor = getScaleFactor(boneTransform.rotationTimestamps[posIdx],
        boneTransform.rotationTimestamps[posIdx + 1], m_currTime);

    glm::quat finalRotation = glm::slerp(boneTransform.rotations[posIdx],
        boneTransform.rotations[posIdx + 1], scaleFactor);

    return glm::toMat4(glm::normalize(finalRotation));
}

glm::mat4 roj::Animator::interpolateScaling( roj::BoneTransform& boneTransform)
{

    int posIdx = getKeyTransformIdx(boneTransform.scaleTimestamps);
    if (posIdx == -1)
        return glm::scale(glm::mat4(1.0f), boneTransform.scales[0]);
    float scaleFactor = getScaleFactor(boneTransform.scaleTimestamps[posIdx],
        boneTransform.scaleTimestamps[posIdx + 1], m_currTime);
    glm::vec3 finalScale = glm::mix(boneTransform.scales[posIdx], boneTransform.scales[posIdx+1], scaleFactor);
    return glm::scale(glm::mat4(1.0f), finalScale);
}

void roj::Animator::calcBoneTransform(BoneNode& node, glm::mat4 offset)
{
    auto& boneInfoMap = m_model->boneInfoMap;
    if (m_currAnim->boneTransforms.find(node.name) != m_currAnim->boneTransforms.end())
    {
        roj::BoneTransform& boneTransform = m_currAnim->boneTransforms.at(node.name);
        glm::mat4 translation = interpolatePosition(boneTransform);
        glm::mat4 rotation = interpolateRotation(boneTransform);
        glm::mat4 scale = interpolateScaling(boneTransform);
        offset *= translation * rotation * scale;
        m_boneMatrices[boneInfoMap[node.name].id] = offset * boneInfoMap[node.name].offset;
    }

    for (roj::BoneNode& child : node.children)
    {
        calcBoneTransform(child, offset);
    }
    
}

roj::Animator::Animator(SkinnedModel& model)
    : m_model(&model)
{
    m_boneMatrices.resize(model.boneInfoMap.size());
}

void roj::Animator::set(const std::string& name)
{
    auto it = m_model->animations.find(name);
    if (it != m_model->animations.end()) {
        m_currAnim = &it->second;
        m_currTime = 0.0f;
    }
}
std::vector<std::string> roj::Animator::get()
{
    std::vector<std::string> animNames;
    animNames.reserve(m_model->animations.size());
    for (auto& [name, _] : m_model->animations)
    {
        animNames.emplace_back(name);
    }

    return animNames;
}

std::vector<glm::mat4>& roj::Animator::getBoneMatrices()
{
    return m_boneMatrices;
}

void roj::Animator::update(float dt)
{
    if (m_currAnim) {
        m_currTime += m_currAnim->ticksPerSec * dt;
        m_currTime = fmod(m_currTime, m_currAnim->duration);
        calcBoneTransform(m_currAnim->rootBone, glm::mat4(1.0f));
    }
}




