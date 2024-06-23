#include "camera.hpp"

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

roj::Camera::Camera(glm::vec3 pos, glm::vec3 worldUp, float yaw, float pitch)
	: m_yaw(yaw), m_pitch(pitch)
	, m_position(pos), m_worldUp(worldUp)
	, m_front(glm::vec3(0.0f, 0.0f, -1.0f)) {update();}

void roj::Camera::placeAt(glm::vec3 pos)
{
    m_position = pos;
}

void roj::Camera::move(Movement dir, float speed, float deltatime)
{
    float velocity = speed * deltatime;
    if (dir == Movement::FORWARD)
        m_position += m_front * velocity;
    if (dir == Movement::BACKWARD)
        m_position -= m_front * velocity;
    if (dir == Movement::LEFT)
        m_position -= m_right * velocity;
    if (dir == Movement::RIGHT)
        m_position += m_right * velocity;
}

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

glm::vec3 roj::Camera::getPosition()
{
    return m_position;
}

glm::mat4 roj::Camera::getViewMatrix()
{
    return glm::lookAt(m_position, m_position + m_front, m_up);
}