#include "animator.hpp"



int roj::Animator::getKeyTransformIdx(float animTime, std::vector<float>& timestamps)
{
    for (int index = 0; index < timestamps.size() - 1; ++index)
    {
        if (animTime < timestamps[index + 1])
            return index;
    }

    return static_cast<int>(timestamps.size() - 1);
}

float roj::Animator::getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
    float scaleFactor = 0.0f;
    float midWayLength = animationTime - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    scaleFactor = midWayLength / framesDiff;
    return scaleFactor;
}

glm::mat4 roj::Animator::interpolatePosition(float animTime, roj::BoneTransform& boneTransform)
{
    if (boneTransform.positions.size() == 1)
        return glm::translate(glm::mat4(1.0f), boneTransform.positions[0]);

    int p0Index = getKeyTransformIdx(animTime, boneTransform.positionTimestamps);
    int p1Index = p0Index + 1;
    float scaleFactor = getScaleFactor(boneTransform.positionTimestamps[p0Index],
        boneTransform.positionTimestamps[p1Index], animTime);
    glm::vec3 finalPosition = glm::mix(boneTransform.positions[p0Index],
        boneTransform.positions[p1Index], scaleFactor);
    return glm::translate(glm::mat4(1.0f), finalPosition);
}
glm::mat4 roj::Animator::interpolateRotation(float animTime, roj::BoneTransform& boneTransform)
{
    if (boneTransform.rotations.size() == 1)
        return glm::toMat4(glm::normalize(boneTransform.rotations[0]));
        
    int p0Index = getKeyTransformIdx(animTime, boneTransform.rotationTimestamps);
    int p1Index = p0Index + 1;
    float scaleFactor = getScaleFactor(boneTransform.rotationTimestamps[p0Index],boneTransform.rotationTimestamps[p1Index], animTime);
    glm::quat finalRotation = glm::slerp(boneTransform.rotations[p0Index],boneTransform.rotations[p1Index], scaleFactor);
    finalRotation = glm::normalize(finalRotation);
    return glm::toMat4(finalRotation);
}
glm::mat4 roj::Animator::interpolateScaling(float animTime, roj::BoneTransform& boneTransform)
{
    if (boneTransform.scales.size() == 1)
        return glm::scale(glm::mat4(1.0f), boneTransform.scales[0]);
    int p0Index = getKeyTransformIdx(animTime, boneTransform.scaleTimestamps);
    int p1Index = p0Index + 1;
    float scaleFactor = getScaleFactor(boneTransform.scaleTimestamps[p0Index], boneTransform.scaleTimestamps[p1Index], animTime);
    glm::vec3 finalScale = glm::mix(boneTransform.scales[p0Index], boneTransform.scales[p1Index], scaleFactor);
    return glm::scale(glm::mat4(1.0f), finalScale);
}

void roj::Animator::calcBoneTransform(BoneNode& node, glm::mat4 offset)
{
    roj::BoneTransform& boneTransform = m_currAnim->boneTransforms[node.name];
    glm::mat4 translation = interpolatePosition(m_currTime, boneTransform);
    glm::mat4 rotation = interpolateRotation(m_currTime, boneTransform);
    glm::mat4 scale = interpolateScaling(m_currTime, boneTransform);
    glm::mat4 globalTransformation = offset * translation * rotation * scale;

    auto& boneInfoMap = m_model->boneInfoMap;
    if (boneInfoMap.find(node.name) != boneInfoMap.end())
    {
        m_boneMatrices[boneInfoMap[node.name].id] = globalTransformation * boneInfoMap[node.name].offset;
    }

    for (roj::BoneNode& child : node.children)
    {
        calcBoneTransform(child, globalTransformation);
    }
}

roj::Animator::Animator(SkinnedModel& model)
    : m_model(&model)
{
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

void roj::Animator::update(float dt)
{
    if (m_currAnim) {
        m_currTime += m_currAnim->ticksPerSec * dt;
        m_currTime = fmod(m_currTime, m_currAnim->duration);
        calcBoneTransform(m_currAnim->rootBone, glm::mat4(1.0f));
    }
}




