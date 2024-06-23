#ifndef MODEL_HPP
#define MODEL_HPP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include "utils.hpp"

namespace roj
{

struct MeshTexture
{
	uint32_t id; aiTextureType type; std::string src;
};

struct Mesh
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoords;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};
	
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<MeshTexture> textures;
	uint32_t VAO = 0;
	void setup();
};

template<typename mesh_t = Mesh, typename model_t = std::vector<mesh_t>>
class ModelLoader
{
private:
	Assimp::Importer m_import;
	model_t m_model;
	std::vector<MeshTexture> m_texCache;
	std::string m_infoLog;
	std::string m_relativeDir;
private:
	void resetLoader();
    void processNode(aiNode* node, const aiScene* scene);
	mesh_t processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<typename mesh_t::Vertex>  getMeshVertices(aiMesh* mesh);
	std::vector<MeshTexture> getMeshTextures(aiMaterial* material);
	std::vector<MeshTexture> loadTextureMap(aiMaterial* mat, aiTextureType type);
public:
	ModelLoader() = default;
	bool load(const std::string& path);
	model_t& get();
	const std::string& getInfoLog();
};


}
#endif //-MODEL_HPP
