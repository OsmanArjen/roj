#ifndef SKINNED_MODEL_HPP
#define SKINNED_MODEL_HPP
#include "model.hpp"
#include <unordered_map>

#define MAX_BONE_INFLUENCE 4

namespace roj
{
	struct SkinnedMesh
	{
		struct Vertex
		{
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec2 texCoords;
			glm::vec3 tangent;
			glm::vec3 bitangent;
			int boneIds[MAX_BONE_INFLUENCE];
			float weights[MAX_BONE_INFLUENCE];
		};

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<MeshTexture> textures;
		uint32_t VAO = 0;
		void setup();
	};

	struct BoneInfo
	{
		int id;
		glm::mat4 offset;
	};

	struct BoneNode
	{
		BoneInfo info;
		std::string name;
		std::vector<BoneNode> children;
	};

	struct BoneTransform {
		std::vector<float> positionTimestamps = {};
		std::vector<float> rotationTimestamps = {};
		std::vector<float> scaleTimestamps = {};

		std::vector<glm::vec3> positions = {};
		std::vector<glm::quat> rotations = {};
		std::vector<glm::vec3> scales = {};
	};

	struct Animation {
		float duration = 0.0f;
		float ticksPerSec = 1.0f;
		BoneNode rootBone;
		std::unordered_map<std::string, BoneTransform> boneTransforms = {};
	};



	struct SkinnedModel
	{
		int boneCount{ 0 };
		std::vector<SkinnedMesh> meshes;
		std::unordered_map<std::string, BoneInfo> boneInfoMap;
		std::unordered_map<std::string, Animation> animations;
		std::vector<SkinnedMesh>::iterator begin();
		std::vector<SkinnedMesh>::iterator end();
		void clear();
	};

}
#endif //-SKINNED_MODEL_HPP

