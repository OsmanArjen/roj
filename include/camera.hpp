#ifndef CAMERA_HPP
#define CAMERA_HPP
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace roj
{
class Camera
{
private:
    static constexpr float DEFAULT_YAW = -90.0f;
    static constexpr float DEFAULT_PITCH = 0.0f;
    static constexpr float PITCH_LIMIT = 89.0f;
    float m_yaw;
    float m_pitch;

    glm::vec3 m_position;
    glm::vec3 m_up;
    glm::vec3 m_worldUp;
    glm::vec3 m_front;
    glm::vec3 m_right;
private:
	void update();
public:
    enum Movement
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };
public:
    Camera(glm::vec3 pos = glm::vec3(0.0f, 1.0f, 0.0f),
           glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = DEFAULT_YAW, float pitch = DEFAULT_PITCH);

    void placeAt(glm::vec3 pos);
	void move(Movement dir, float speed, float deltatime);
	void rotate(float xoffset, float yoffset, float sensitivity, bool limitPitch = true);
    glm::vec3 getPosition();
    glm::mat4 getViewMatrix();


};
}
#endif //-CAMERA_HPP
