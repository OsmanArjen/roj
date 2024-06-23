#include <shader.hpp>

namespace roj
{

unsigned int GLShaderObject::createShader(GLenum type, const std::string& file)
{
	std::string source;
	if (!utils::fileio::read(file, source))
	{
		infoLog = (infoLog + "Couldn't read the shader file:'" + file + "'\n");
		return 0;
	}
	const char* sourceCStr = source.c_str();
	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &sourceCStr, NULL);
	glCompileShader(shader);

	int success;
	char log[1024];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	glGetShaderInfoLog(shader, 1024, NULL, log);
	if (!success)
	{
		infoLog = infoLog + ((type == GL_VERTEX_SHADER) ? "vertex" : "fragment") + log;
		return 0;
	}

	return shader;


}

GLShaderObject::GLShaderObject()
	: id(glCreateProgram()) {}

std::string GLShaderObject::getInfoLog()
{
	return infoLog;
}

bool GLShaderObject::link(const std::string& vsPath, const std::string& fsPath)
{
	unsigned int shader_vs{ createShader(GL_VERTEX_SHADER, vsPath) };
	unsigned int shader_fs{ createShader(GL_FRAGMENT_SHADER, fsPath) };
	if (!shader_vs || !shader_fs)
		return false;

	glAttachShader(id, shader_vs);
	glAttachShader(id, shader_fs);
	glLinkProgram(id);

	int success;
	char log[1024];
 	glGetProgramiv(id, GL_LINK_STATUS, &success);
	glGetProgramInfoLog(id, 1024, NULL, log);
	if(!success)
 		infoLog += log;

   	return success;
}

//
void roj::GLShaderObject::uniform1i(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(id, name.c_str()), value);
}
void roj::GLShaderObject::uniform1f(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(id, name.c_str()), value);
}
void roj::GLShaderObject::uniform2f(const std::string& name, const glm::vec2& value) const
{
    glUniform2f(glGetUniformLocation(id, name.c_str()), value.x, value.y);
}
void roj::GLShaderObject::uniform3f(const std::string& name, const glm::vec3& value) const
{
    glUniform3f(glGetUniformLocation(id, name.c_str()), value.x, value.y, value.z);
}
void roj::GLShaderObject::uniform4f(const std::string& name, const glm::vec4& value) const
{
    glUniform4f(glGetUniformLocation(id, name.c_str()), value.x, value.y, value.z, value.w);
}
void roj::GLShaderObject::uniformMat2(const std::string& name, const glm::mat2& mat) const
{
    glUniformMatrix2fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void roj::GLShaderObject::uniformMat3(const std::string& name, const glm::mat3& mat) const
{
    glUniformMatrix3fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void roj::GLShaderObject::uniformMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void GLShaderObject::use()
{
	glUseProgram(id);
}

}
