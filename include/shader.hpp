#ifndef SHADER_HPP
#define SHADER_HPP
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include "utils.hpp"

namespace roj
{

	class GLShaderObject
	{
	private:
		unsigned int id;
		std::string infoLog;
	private:
		unsigned int createShader(GLenum type, const std::string& file);
	public:
		GLShaderObject();
		std::string getInfoLog();
		bool link(const std::string& vsPath, const std::string& fsPath);
		void uniform1i(const std::string& name, int value) const;
		void uniform1f(const std::string& name, float value) const;
		void uniform2f(const std::string& name, const glm::vec2& value) const;
		void uniform3f(const std::string& name, const glm::vec3& value) const;
		void uniform4f(const std::string& name, const glm::vec4& value) const;
		void uniformMat2(const std::string& name, const glm::mat2& mat) const;
		void uniformMat3(const std::string& name, const glm::mat3& mat) const;
		void uniformMat4(const std::string& name, const glm::mat4& mat) const;
		void use();

	};
}
#endif //-SHADER_HPP
