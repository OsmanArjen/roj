#define STB_IMAGE_IMPLEMENTATION
#include <utils.hpp>

namespace utils
{
	bool fileio::read(const std::string& path, std::string& data)
	{
        std::ifstream file(path);
        if (!file.is_open())
            return false;

        std::stringstream buffer;
        buffer << file.rdbuf();
        data = buffer.str();
        file.close();

        return true;
	}

    glm::mat4 assimp::toGlmMat4(aiMatrix4x4 mat) {
        glm::mat4 m;
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                m[x][y] = mat[y][x];
            }
        }
        return m;
    }

    glm::vec3 assimp::toGlmVec3(aiVector3D vec) {
        return glm::vec3(vec.x, vec.y, vec.z);
    }

    glm::quat assimp::toGlmQuat(aiQuaternion quat) {
        glm::quat q;
        q.x = quat.x;
        q.y = quat.y;
        q.z = quat.z;
        q.w = quat.w;

        return q;
    }

    unsigned int loadGLTexture(const std::string& path)
    {
        unsigned int texId;
        glGenTextures(1, &texId);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
        if(!data)
        {
            stbi_image_free(data);
            return 0;
        }

        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        return texId;
    }
}