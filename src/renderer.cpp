#include "renderer.hpp"


void game::renderModel(std::vector<roj::Mesh>& model, roj::GLShaderObject& shader)
{
    for (roj::Mesh& mesh : model)
    {
        auto& textures = mesh.textures;
        auto& indices = mesh.indices;
        auto& vertices = mesh.vertices;
        uint32_t VAO = mesh.VAO;

        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;
        for (unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i); 
            std::string uniformStr;
            if (textures[i].type == aiTextureType_DIFFUSE)
                uniformStr = "texture_diffuse" + std::to_string(diffuseNr++);
            else if (textures[i].type == aiTextureType_SPECULAR)
                uniformStr = "texture_specular" + std::to_string(specularNr++); // transfer unsigned int to string
            else if (textures[i].type == aiTextureType_NORMALS)
                uniformStr = "texture_normal" + std::to_string(normalNr++); // transfer unsigned int to string
            else if (textures[i].type == aiTextureType_HEIGHT)
                uniformStr = "texture_normal" + std::to_string(heightNr++); // transfer unsigned int to string

            shader.uniform1i(uniformStr.c_str(), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }
}

