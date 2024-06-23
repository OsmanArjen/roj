#include "model.hpp"
#include <filesystem>
namespace roj
{
template class ModelLoader<Mesh>;

void Mesh::setup()
{
    unsigned int VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
    
    glBindVertexArray(0);
}



template<typename mesh_t, typename model_t>
model_t& ModelLoader<mesh_t, model_t>::get()
{
    return m_model;
}

template<typename mesh_t, typename model_t>
const std::string& ModelLoader<mesh_t, model_t>::getInfoLog()
{
    return m_infoLog;
}

template<typename mesh_t, typename model_t>
std::vector<MeshTexture> ModelLoader<mesh_t, model_t>::getMeshTextures(aiMaterial* material)
{
    std::vector<MeshTexture> textures;
    std::vector<MeshTexture> diffuseMaps = loadTextureMap(material, aiTextureType_DIFFUSE);
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    std::vector<MeshTexture> specularMaps = loadTextureMap(material, aiTextureType_SPECULAR);
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    std::vector<MeshTexture> normalMaps = loadTextureMap(material, aiTextureType_NORMALS);
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    std::vector<MeshTexture> heightMaps = loadTextureMap(material, aiTextureType_HEIGHT);
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
    return textures;
}

template<typename mesh_t, typename model_t>
std::vector<MeshTexture> ModelLoader<mesh_t, model_t>::loadTextureMap(aiMaterial* mat, aiTextureType type)
{
    std::vector<MeshTexture> textures;
    for (uint32_t i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString texSrc;
        mat->GetTexture(type, i, &texSrc);
        bool cached = false;
        for (uint32_t j = 0; !cached && (j < m_texCache.size()); j++)
        {
            if (std::strcmp(m_texCache[j].src.data(), texSrc.C_Str()) == 0)
            {
                textures.push_back(m_texCache[j]);
                cached = true;
            }
        }

        if (!cached)
        {
            std::string texPath = (std::filesystem::path(m_relativeDir) / texSrc.C_Str()).string();
            m_texCache.emplace_back(utils::loadGLTexture(texPath), type, texSrc.C_Str());
        }
    }

    return textures;
}

template<typename mesh_t, typename model_t>
void ModelLoader<mesh_t, model_t>::resetLoader()
{
    m_texCache.clear();
    m_model.clear();
    m_infoLog.clear();
    m_relativeDir.clear();
}


bool ModelLoader<Mesh>::load(const std::string& path)
{
    resetLoader();
    const aiScene* scene = m_import.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    m_relativeDir = static_cast<std::filesystem::path>(path).parent_path().string();

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        m_infoLog += m_import.GetErrorString();
        return false;
    }

    processNode(scene->mRootNode, scene);
    for (Mesh& mesh : m_model)
    {
        mesh.setup();
    }
    return true;
}

void ModelLoader<Mesh>::processNode(aiNode* node, const aiScene* scene)
{
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_model.push_back(processMesh(mesh, scene));
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh ModelLoader<Mesh>::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<MeshTexture> textures = getMeshTextures(scene->mMaterials[mesh->mMaterialIndex]);
    std::vector<Mesh::Vertex>  vertices = getMeshVertices(mesh);
    std::vector<uint32_t> indices;
    for (uint32_t i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace& face = mesh->mFaces[i];
        indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
    }
    return Mesh{ vertices, indices, textures };
}

std::vector<Mesh::Vertex> ModelLoader<Mesh>::getMeshVertices(aiMesh* mesh)
{
    std::vector<Mesh::Vertex> vertices;
    for (uint32_t i = 0; i < mesh->mNumVertices; i++)
    {
        Mesh::Vertex vertex;
        vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        if (mesh->HasNormals())
        {
            vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        }
        if (mesh->mTextureCoords[0])
        {
            vertex.texCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
            vertex.tangent = { mesh->mTangents[i].x,   mesh->mTangents[i].y,   mesh->mTangents[i].z };
            vertex.bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
        }
        else
            vertex.texCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }

    return vertices;
}





}