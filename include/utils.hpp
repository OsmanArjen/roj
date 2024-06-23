#ifndef UTILS_HPP
#define UTILS_HPP
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h>    
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/scene.h>

namespace utils
{
	namespace fileio
	{
		bool read(const std::string& path, std::string& data);
	}

	namespace assimp
	{
		glm::mat4 toGlmMat4(aiMatrix4x4 mat);
		glm::vec3 toGlmVec3(aiVector3D vec);
		glm::quat toGlmQuat(aiQuaternion quat);
	}

	unsigned int loadGLTexture(const std::string& path);
}

#endif //-UTILS_HPP
