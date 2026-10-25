#include <asset/import.h>
#include <gfx/resource/mesh.h>
#include <bimg/bimg.h>
#include <bimg/encode.h>
#include <bimg/decode.h>
#include <bx/allocator.h>
#include <entity/transform.h>
#include <entity/animation.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <entt/container/dense_map.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>
#include <stb/stb_image.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <asset/resource.h>
namespace {

    // Maps an assimp node to its position in ModelAsset::nodes (pre-order).
    using NodeIndexMap = entt::dense_map<const aiNode*, entt::id_type>;

    constexpr unsigned int kImportFlags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |       // only generates normals for meshes that lack them
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_PopulateArmatureData |   // fills aiBone::mNode, which mapBonesToJoints relies on
        aiProcess_LimitBoneWeights;        // max 4 influences per vertex, matching SkinData

    constexpr int kRgbaChannels = 4;
    constexpr uint32_t kInvalidJoint = UINT32_MAX;

    // ---------------------------------------------------------------------------
    // assimp -> glm
    // ---------------------------------------------------------------------------

    glm::vec3 toVec3(const aiVector3D& v) { return { v.x, v.y, v.z }; }

    // Texture coordinates are stored as aiVector3D; only x and y are used.
    glm::vec2 toVec2(const aiVector3D& v) { return { v.x, v.y }; }

    glm::mat4 toMat4(const aiMatrix4x4& m) {
        // assimp is row-major, glm is column-major: element (row, col) lands at [col][row].
        glm::mat4 result;
        for (int row = 0; row < 4; row++)
            for (int col = 0; col < 4; col++)
                result[col][row] = m[row][col];
        return result;
    }

    glm::quat toQuat(const aiQuaternion& q) { return { q.w, q.x, q.y, q.z }; } // glm's constructor takes w first

    template <typename T, typename Key, typename Convert>
    void convertTrack(roj::AnimationTrack<T>& track, const Key* keys, uint32_t count,
        double secondsPerTick, Convert convert) {
        track.times.reserve(count);
        track.values.reserve(count);
        for (uint32_t i = 0; i < count; i++) {
            track.times.push_back(static_cast<float>(keys[i].mTime * secondsPerTick));
            track.values.push_back(convert(keys[i].mValue));
        }
    }

    std::optional<roj::AnimationChannel> convertChannel(const aiNodeAnim& ainode, const aiNode& root,
        const NodeIndexMap& nodeIndices, double secondsPerTick) {
        // Unlike bones, channels carry only a node *name*, so look the node up by name.
        const aiNode* target = root.FindNode(ainode.mNodeName);
        const auto entry = target ? nodeIndices.find(target) : nodeIndices.end();
        if (entry == nodeIndices.end())
            return std::nullopt;

        roj::AnimationChannel channel;
        channel.node = entry->second;
        convertTrack(channel.position, ainode.mPositionKeys, ainode.mNumPositionKeys, secondsPerTick, toVec3);
        convertTrack(channel.rotation, ainode.mRotationKeys, ainode.mNumRotationKeys, secondsPerTick, toQuat);
        convertTrack(channel.scale, ainode.mScalingKeys, ainode.mNumScalingKeys, secondsPerTick, toVec3);
        return channel;
    }

    roj::AnimationClip convertAnimation(roj::ModelAsset& model, const aiNode& root,
        const aiAnimation& aianim, const NodeIndexMap& nodeIndices) {
        // assimp times are in ticks; mTicksPerSecond is 0 when the file doesn't say.
        const double ticksPerSecond = aianim.mTicksPerSecond > 0.0 ? aianim.mTicksPerSecond : 25.0;
        const double secondsPerTick = 1.0 / ticksPerSecond;

        roj::AnimationClip clip;
        clip.name = aianim.mName.C_Str();
        clip.duration = static_cast<float>(aianim.mDuration * secondsPerTick);
        for (uint32_t c = 0; c < aianim.mNumChannels; c++) {
            if (auto channel = convertChannel(*aianim.mChannels[c], root, nodeIndices, secondsPerTick))
                clip.channels.push_back(std::move(*channel));
            else
                model.log += "Animation channel has no node: " + std::string(aianim.mChannels[c]->mNodeName.C_Str()) + "\n";
        }
        return clip;
    }

    void loadAnimations(roj::ModelAsset& model, const aiScene& scene, const NodeIndexMap& nodeIndices) {
        model.animations.resize(scene.mNumAnimations);
        for (uint32_t i = 0; i < scene.mNumAnimations; i++) {
            model.animations[i] = convertAnimation(model, *scene.mRootNode, *scene.mAnimations[i], nodeIndices);
        }
    }
    // ---------------------------------------------------------------------------
    // Vertex formats
    // ---------------------------------------------------------------------------

    struct VertexData {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texcoord;
        glm::vec3 tangent;
        glm::vec3 bitangent;

        static std::vector<roj::GfxVertexAttribute> layout() {
            return {
                { roj::GfxVertexSemantic::Position,  roj::GfxVertexFormat::Float, 3, false },
                { roj::GfxVertexSemantic::Normal,    roj::GfxVertexFormat::Float, 3, false },
                { roj::GfxVertexSemantic::TexCoord0, roj::GfxVertexFormat::Float, 2, false },
                { roj::GfxVertexSemantic::Tangent,   roj::GfxVertexFormat::Float, 3, false },
                { roj::GfxVertexSemantic::Bitangent, roj::GfxVertexFormat::Float, 3, false }
            };
        }
    };


    struct SkinData {
        static constexpr uint32_t kMaxInfluences = 4;

        float joints[kMaxInfluences];    // joint ids as floats: exact up to 2^24, see below
        float weights[kMaxInfluences];

        static std::vector<roj::GfxVertexAttribute> layout() {
            return {
                { roj::GfxVertexSemantic::Indices, roj::GfxVertexFormat::Float, kMaxInfluences, false },
                { roj::GfxVertexSemantic::Weight,  roj::GfxVertexFormat::Float, kMaxInfluences, false }
            };
        }

        // Keeps the strongest influences: a new one only gets in by displacing the weakest.
        void addInfluence(uint32_t joint, float weight) {
            uint32_t weakest = 0;
            for (uint32_t k = 1; k < kMaxInfluences; k++)
                if (weights[k] < weights[weakest])
                    weakest = k;

            if (weight > weights[weakest]) {
                joints[weakest] = static_cast<float>(joint);   // the only changed line
                weights[weakest] = weight;
            }
        }
    };

    // ---------------------------------------------------------------------------
    // Meshes
    // ---------------------------------------------------------------------------

    roj::GfxMeshData convertMesh(const aiMesh& aimesh) {
        const bool hasUV0 = aimesh.mTextureCoords[0] != nullptr;
        const bool hasTangents = aimesh.mTangents != nullptr && aimesh.mBitangents != nullptr;

        std::vector<VertexData> vertices;
        vertices.reserve(aimesh.mNumVertices);
        for (uint32_t i = 0; i < aimesh.mNumVertices; i++) {
            VertexData vertex{};
            vertex.position = toVec3(aimesh.mVertices[i]);
            vertex.normal = toVec3(aimesh.mNormals[i]);
            vertex.texcoord = hasUV0 ? toVec2(aimesh.mTextureCoords[0][i]) : glm::vec2{ 0.0f };
            vertex.tangent = hasTangents ? toVec3(aimesh.mTangents[i]) : glm::vec3{ 0.0f };
            vertex.bitangent = hasTangents ? toVec3(aimesh.mBitangents[i]) : glm::vec3{ 0.0f };
            vertices.push_back(vertex);
        }

        roj::GfxMeshData meshData;

        // Each face's indices are emitted in reverse order, which flips the winding.
        for (uint32_t f = 0; f < aimesh.mNumFaces; f++) {
            const aiFace& face = aimesh.mFaces[f];
            std::reverse_copy(face.mIndices, face.mIndices + face.mNumIndices,
                std::back_inserter(meshData.indices));
        }

        meshData.layout = VertexData::layout();
        meshData.vertices.resize(vertices.size() * sizeof(VertexData));
        std::memcpy(meshData.vertices.data(), vertices.data(), meshData.vertices.size());
        return meshData;
    }
    roj::Transform decomposeLocal(const glm::mat4& local) {
        roj::Transform transform{};
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(local, transform.scale, transform.rotation, transform.position, skew, perspective);
        return transform;
    }

   
    uint32_t addNode(roj::NodeTable& nodes, std::string name, entt::id_type parent, const roj::Transform& local) {
        const uint32_t row = static_cast<uint32_t>(nodes.names.size());
        nodes.names.push_back(std::move(name));
        nodes.parents.push_back(parent);
        nodes.subtrees.push_back(1);              // a leaf until its children are added
        nodes.locals.push_back(local);
        nodes.meshes.push_back(entt::null);
        nodes.materials.push_back(entt::null);
        nodes.skinned.push_back(0);
        return row;
    }
    void setMesh(roj::NodeTable& nodes, uint32_t row, const aiScene& scene, uint32_t meshIndex) {
        const aiMesh& aimesh = *scene.mMeshes[meshIndex];
        nodes.meshes[row] = meshIndex;
        nodes.materials[row] = aimesh.mMaterialIndex;
        nodes.skinned[row] = aimesh.HasBones();
    }
    void convertNode(roj::ModelAsset& model, const aiScene& scene, const aiNode& ainode, entt::id_type parent, NodeIndexMap& indices) {
        roj::NodeTable& nodes = model.nodes;

        const uint32_t row = addNode(nodes, ainode.mName.C_Str(), parent, decomposeLocal(toMat4(ainode.mTransformation)));
        indices.emplace(&ainode, row);

        // One mesh per row: the first rides on the node itself, any others become
        // identity-transform leaves directly below it (still pre-order, still inside its subtree).
        for (uint32_t m = 0; m < ainode.mNumMeshes; m++) {
            const uint32_t meshRow = m == 0
                ? row
                : addNode(nodes, nodes.names[row] + "#" + std::to_string(m), row, decomposeLocal(glm::mat4(1.0f)));
            setMesh(nodes, meshRow, scene, ainode.mMeshes[m]);
        }

        for (unsigned int c = 0; c < ainode.mNumChildren; c++)
            convertNode(model, scene, *ainode.mChildren[c], row, indices);

        // Every row appended since entering this node belongs to its subtree.
        nodes.subtrees[row] = static_cast<uint32_t>(nodes.names.size()) - row;
    }
    NodeIndexMap convertNodeTree(roj::ModelAsset& model, const aiScene& scene) {
        NodeIndexMap indices;
        convertNode(model, scene, *scene.mRootNode, entt::null, indices);
        return indices;
    }
    uint32_t findOrAddJoint(roj::ModelAsset& model, entt::id_type nodeIdx, const aiMatrix4x4& offsetMatrix) {
        auto& joints = model.joints;

        const auto existing = std::find_if(joints.begin(), joints.end(),
            [&](const roj::ModelJoint& joint) { return joint.node == nodeIdx; });
        if (existing != joints.end())
            return static_cast<uint32_t>(existing - joints.begin());

        joints.push_back({ nodeIdx, toMat4(offsetMatrix) });
        return static_cast<uint32_t>(joints.size() - 1);
    }
    std::vector<uint32_t> mapBonesToJoints(roj::ModelAsset& model, const aiMesh& aimesh, const NodeIndexMap& nodeIndices) {
        std::vector<uint32_t> boneToJoint(aimesh.mNumBones, kInvalidJoint);

        for (uint32_t b = 0; b < aimesh.mNumBones; b++) {
            const aiBone& bone = *aimesh.mBones[b];
            const auto entry = bone.mNode ? nodeIndices.find(bone.mNode) : nodeIndices.end();

            if (entry == nodeIndices.end()) {
                model.log += "Bone has no node: " + std::string(bone.mName.C_Str()) + "\n";
                continue;
            }
            boneToJoint[b] = findOrAddJoint(model, entry->second, bone.mOffsetMatrix);
        }
        return boneToJoint;
    }
    std::vector<SkinData> buildSkinData(const aiMesh& aimesh, const std::vector<uint32_t>& boneToJoint) {
        std::vector<SkinData> skin(aimesh.mNumVertices); // zero-initialised

        for (uint32_t b = 0; b < aimesh.mNumBones; b++) {
            if (boneToJoint[b] == kInvalidJoint)
                continue;

            const aiBone& bone = *aimesh.mBones[b];
            for (uint32_t w = 0; w < bone.mNumWeights; w++) {
                const aiVertexWeight& weight = bone.mWeights[w];
                skin[weight.mVertexId].addInfluence(boneToJoint[b], weight.mWeight);
            }
        }
        return skin;
    }
    void interleaveSkinData(roj::GfxMeshData& meshData, const std::vector<SkinData>& skin) {
        constexpr size_t kBaseSize = sizeof(VertexData);
        constexpr size_t kStride = kBaseSize + sizeof(SkinData);

        std::vector<std::byte> interleaved(skin.size() * kStride);
        for (size_t i = 0; i < skin.size(); i++) {
            std::byte* dst = interleaved.data() + i * kStride;
            std::memcpy(dst, meshData.vertices.data() + i * kBaseSize, kBaseSize);
            std::memcpy(dst + kBaseSize, &skin[i], sizeof(SkinData));
        }
        meshData.vertices = std::move(interleaved);

        const auto skinLayout = SkinData::layout();
        meshData.layout.insert(meshData.layout.end(), skinLayout.begin(), skinLayout.end());
    }
    roj::GfxMeshData convertSkinnedMesh(const aiMesh& aimesh, const std::vector<uint32_t>& boneToJoint) {
        roj::GfxMeshData meshData = convertMesh(aimesh);
        interleaveSkinData(meshData, buildSkinData(aimesh, boneToJoint));
        return meshData;
    }
    void loadMeshes(roj::ModelAsset& model, const aiScene& scene, const NodeIndexMap& nodeIndices) {
        for (uint32_t i = 0; i < scene.mNumMeshes; i++) {
            const aiMesh& aimesh = *scene.mMeshes[i];

            model.meshes[i] = (aimesh.HasBones()
                ? convertSkinnedMesh(aimesh, mapBonesToJoints(model, aimesh, nodeIndices))
                : convertMesh(aimesh));
        }
    }




    size_t embeddedByteSize(const aiTexture& t) {
        return t.mHeight == 0 ? t.mWidth : size_t(t.mWidth) * t.mHeight * sizeof(aiTexel);
    }

    void importEmbeddedTextures(roj::ModelAsset& model, const aiScene& scene) {
        model.embedded.resize(scene.mNumTextures);   // one slot per aiTexture so "*N" indexes directly
        for (unsigned i = 0; i < scene.mNumTextures; ++i) {
            const aiTexture& tex = *scene.mTextures[i];
            const size_t size = embeddedByteSize(tex);
            if (!tex.pcData || size == 0) {
                model.log += "Embedded texture has no data: *" + std::to_string(i) + "\n";
                continue;                            // slot stays empty
            }
            roj::EmbeddedTexture& out = model.embedded[i];
            out.width = tex.mWidth;
            out.height = tex.mHeight;
            out.format.assign(tex.achFormatHint, strnlen(tex.achFormatHint, sizeof(tex.achFormatHint)));
            const auto* bytes = reinterpret_cast<const uint8_t*>(tex.pcData);
            out.data.assign(bytes, bytes + size);
        }
    }

    void importMaterials(roj::ModelAsset& model, const aiScene& scene) {
        for (unsigned i = 0; i < scene.mNumMaterials; ++i) {
            auto mat = std::make_shared<aiMaterial>();
            aiMaterial::CopyPropertyList(mat.get(), scene.mMaterials[i]);
            model.materials.push_back(std::move(mat));
        }
    }

} // namespace

namespace roj {

    std::optional<ModelAsset> importModel(const std::filesystem::path& path) {
        thread_local Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.generic_string(), kImportFlags);

        ModelAsset model;
        model.path = path.generic_string();
        model.log += "Loading model: " + model.path + "\n";

        const bool importFailed = !scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode;
        if (importFailed) {
            model.log += importer.GetErrorString();
            return model;
        }

        model.meshes.resize(scene->mNumMeshes);
        model.materials.reserve(scene->mNumMaterials);
        const NodeIndexMap nodeIndices = convertNodeTree(model, *scene);

        loadMeshes(model, *scene, nodeIndices);
        loadAnimations(model, *scene, nodeIndices);
        importEmbeddedTextures(model, *scene);
        importMaterials(model, *scene);
        return model;
    }

} // namespace roj