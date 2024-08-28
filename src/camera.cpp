#include "camera.hpp"
#include <iostream>
void roj::Camera::update()
{
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);

    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

roj::Camera::Camera(glm::vec3 worldUp, float yaw, float pitch)
	: m_yaw(yaw), m_pitch(pitch)
	, m_worldUp(worldUp)
	, m_front(glm::vec3(0.0f, 0.0f, -1.0f)) {update();}

void roj::Camera::rotate(float xoffset, float yoffset, float sensitivity, bool limitPitch)
{
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    if (limitPitch)
    {
        if (m_pitch > PITCH_LIMIT)
            m_pitch = PITCH_LIMIT;

        if (m_pitch < -PITCH_LIMIT)
            m_pitch = -PITCH_LIMIT;
    }

    update();
}

float roj::Camera::getYaw()
{
    return m_yaw;
}

float roj::Camera::getPitch()
{
    return m_pitch;
}

glm::vec3 roj::Camera::getFront()
{
    return m_front;
}

glm::vec3 roj::Camera::getRight()
{
    return m_right;
}

glm::mat4 roj::Camera::getViewMatrix(glm::vec3 pos)
{
    return glm::lookAt(pos, pos + m_front, m_up);
}