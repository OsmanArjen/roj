#include <asset/model.h>
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
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <asset/resource.h>
namespace {

    // Maps an assimp node to its position in ModelScene::nodes (pre-order).
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

    roj::AnimationClip convertAnimation(roj::ModelScene& model, const aiNode& root,
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

    void loadAnimations(roj::ModelScene& model, entt::registry& reg, const aiScene& scene,
        const NodeIndexMap& nodeIndices) {
        model.animations.resize(scene.mNumAnimations);
        for (uint32_t i = 0; i < scene.mNumAnimations; i++) {
            model.animations[i] = reg.create();
            reg.emplace<roj::AnimationClip>(model.animations[i],
                convertAnimation(model, *scene.mRootNode, *scene.mAnimations[i], nodeIndices));
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

    // ---------------------------------------------------------------------------
    // Node tree
    //
    // Nodes are stored in ModelScene::nodes in pre-order, and a node's index is its
    // position in that walk. Joints refer to nodes by this index, so convertNode and
    // indexNodes must visit nodes in the same order.
    // ---------------------------------------------------------------------------

        // ---------------------------------------------------------------------------
    // Node tree
    //
    // Rows of ModelScene::nodes are in pre-order. Joints and animation channels refer to
    // nodes by row, so the walk builds the table and the aiNode -> row map together.
    // ---------------------------------------------------------------------------

    roj::Transform decomposeLocal(const glm::mat4& local) {
        roj::Transform transform{};
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(local, transform.scale, transform.rotation, transform.position, skew, perspective);
        return transform;
    }

    // The only function that knows every column, so they cannot get out of step.
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

    // Material and skinning are properties of the mesh, so they are set together with it.
    void setMesh(roj::NodeTable& nodes, uint32_t row, const aiScene& scene, uint32_t meshIndex) {
        const aiMesh& aimesh = *scene.mMeshes[meshIndex];
        nodes.meshes[row] = meshIndex;
        nodes.materials[row] = aimesh.mMaterialIndex;
        nodes.skinned[row] = aimesh.HasBones();
    }

    void convertNode(roj::ModelScene& model, const aiScene& scene, const aiNode& ainode,
        entt::id_type parent, NodeIndexMap& indices) {
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

    NodeIndexMap convertNodeTree(roj::ModelScene& model, const aiScene& scene) {
        NodeIndexMap indices;
        convertNode(model, scene, *scene.mRootNode, entt::null, indices);
        return indices;
    }

    // ---------------------------------------------------------------------------
    // Skinning
    // ---------------------------------------------------------------------------

    uint32_t findOrAddJoint(roj::ModelScene& model, entt::id_type nodeIdx, const aiMatrix4x4& offsetMatrix) {
        auto& joints = model.joints;

        const auto existing = std::find_if(joints.begin(), joints.end(),
            [&](const roj::ModelJoint& joint) { return joint.node == nodeIdx; });
        if (existing != joints.end())
            return static_cast<uint32_t>(existing - joints.begin());

        joints.push_back({ nodeIdx, toMat4(offsetMatrix) });
        return static_cast<uint32_t>(joints.size() - 1);
    }


    std::vector<uint32_t> mapBonesToJoints(roj::ModelScene& model, const aiMesh& aimesh, const NodeIndexMap& nodeIndices) {
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

    // Rewrites the vertex buffer as [VertexData | SkinData] per vertex and extends the layout to match.
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

    // ---------------------------------------------------------------------------
   // Textures
   // ---------------------------------------------------------------------------


    bx::AllocatorI* imageAllocator() {
        static bx::DefaultAllocator allocator;
        return &allocator;
    }
    std::optional<std::shared_ptr<bimg::ImageContainer>> withMips(bimg::ImageContainer* decoded) {
        const std::shared_ptr<bimg::ImageContainer> base(decoded, bimg::imageFree); // freed once the mipped copy exists
        if (!base)
            return std::nullopt;
        std::shared_ptr<bimg::ImageContainer> mipped(bimg::imageGenerateMips(imageAllocator(), *base), bimg::imageFree);
        if (!mipped)
            return std::nullopt;
        return mipped;
    }


    std::optional<std::shared_ptr<bimg::ImageContainer>> decodeImage(const void* bytes, size_t size) {
        if (size == 0)
            return std::nullopt;
        return withMips(bimg::imageParse(imageAllocator(), bytes, static_cast<uint32_t>(size), bimg::TextureFormat::RGBA8));
    }

    std::optional<std::shared_ptr<bimg::ImageContainer>> loadTextureFile(const std::filesystem::path& file) {
        std::ifstream stream(file, std::ios::binary);
        const std::vector<char> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        return decodeImage(bytes.data(), bytes.size());
    }

    std::optional<std::shared_ptr<bimg::ImageContainer>> decodeCompressedTexture(const aiTexture& embedded) {
        return decodeImage(embedded.pcData, embedded.mWidth);
    }


    std::optional<std::shared_ptr<bimg::ImageContainer>> copyRawTexture(const aiTexture& embedded) {
        bimg::ImageContainer* raw = bimg::imageAlloc(imageAllocator(), bimg::TextureFormat::RGBA8,
            embedded.mWidth, embedded.mHeight, 1, 1, false, false);
        if (!raw)
            return std::nullopt;

        auto* texels = static_cast<uint8_t*>(raw->m_data);
        const size_t texelCount = static_cast<size_t>(embedded.mWidth) * embedded.mHeight;
        for (size_t t = 0; t < texelCount; t++) {
            const aiTexel& texel = embedded.pcData[t];
            const size_t base = t * kRgbaChannels;
            texels[base + 0] = texel.r;
            texels[base + 1] = texel.g;
            texels[base + 2] = texel.b;
            texels[base + 3] = texel.a;
        }
        return withMips(raw);
    }

    std::optional<std::shared_ptr<bimg::ImageContainer>> readEmbeddedTexture(const aiTexture& embedded) {
        const bool isCompressed = embedded.mHeight == 0;
        if (isCompressed)
            return decodeCompressedTexture(embedded);
        return copyRawTexture(embedded);
    }

    entt::entity loadMaterialTexture(roj::ModelScene& model, entt::registry& reg,
        const std::filesystem::path& modelDir,
        const aiMaterial& aimat, aiTextureType type) {
        if (aimat.GetTextureCount(type) == 0)
            return entt::null;

        aiString name;
        aimat.GetTexture(type, 0, &name);
        auto strkey = model.path + ":" + name.C_Str();
        const entt::id_type key = entt::hashed_string(strkey.c_str());
        const auto cached = roj::resourceFind<std::shared_ptr<bimg::ImageContainer>>(reg, key);
        if (cached != entt::null) return cached;
        model.textures.emplace_back(key);
        return roj::resourceLoad<std::shared_ptr<bimg::ImageContainer>>(reg, key, (modelDir / name.C_Str()));
    }

    roj::GfxPbrMaterial convertMaterial(roj::ModelScene& model, entt::registry& reg, const aiMaterial& aimat) {
        const std::filesystem::path modelDir = std::filesystem::path(model.path).parent_path();
        auto load = [&](aiTextureType type) {
            return loadMaterialTexture(model, reg, modelDir, aimat, type);
            };

        roj::GfxPbrMaterial material;
        material.albedo = load(aiTextureType_BASE_COLOR);
        material.normal = load(aiTextureType_NORMALS);
        material.metallicRoughness = load(aiTextureType_GLTF_METALLIC_ROUGHNESS);
        material.emissive = load(aiTextureType_EMISSIVE);
		material.occlusion = load(aiTextureType_LIGHTMAP);
        aiColor4D color;
        if (aimat.Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS)
            material.baseColor = { color.r, color.g, color.b, color.a };

        aiColor3D emissive;
        if (aimat.Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
            material.emissiveFactor = { emissive.r, emissive.g, emissive.b };

        aimat.Get(AI_MATKEY_METALLIC_FACTOR, material.metallicFactor);
        aimat.Get(AI_MATKEY_ROUGHNESS_FACTOR, material.roughnessFactor);
        

        return material;
    }

    // ---------------------------------------------------------------------------
    // Scene loading steps (called in order by loadModel)
    // ---------------------------------------------------------------------------

    void loadMeshes(roj::ModelScene& model, entt::registry& reg, const aiScene& scene,
        const NodeIndexMap& nodeIndices) {
        for (uint32_t i = 0; i < scene.mNumMeshes; i++) {
            const aiMesh& aimesh = *scene.mMeshes[i];

            model.meshes[i] = reg.create();
            reg.emplace<roj::GfxMeshData>(model.meshes[i], aimesh.HasBones()
                ? convertSkinnedMesh(aimesh, mapBonesToJoints(model, aimesh, nodeIndices))
                : convertMesh(aimesh));
        }
    }

    void loadEmbeddedTextures(roj::ModelScene& model, entt::registry& reg, const aiScene& scene) {
        for (uint32_t i = 0; i < scene.mNumTextures; i++) {
            auto strkey = model.path + ":" + ("*" + std::to_string(i));
            auto key = entt::hashed_string(strkey.c_str());
            roj::resourceLoad<std::shared_ptr<bimg::ImageContainer>>(reg, key, std::ref(*scene.mTextures[i]));
            model.textures.emplace_back(key);
        }
    }

    void loadMaterials(roj::ModelScene& model, entt::registry& reg, const aiScene& scene) {
        for (uint32_t i = 0; i < scene.mNumMaterials; i++)
            model.materials[i] = convertMaterial(model, reg, *scene.mMaterials[i]);
    }


    // ---------------------------------------------------------------------------
    // Instantiation
    // ---------------------------------------------------------------------------


    std::vector<entt::entity> instantiateNodes(entt::registry& reg, const roj::ModelScene& model,
        entt::entity rootEntity) {
        const roj::NodeTable& nodes = model.nodes;

        // Row 0 is the caller's entity; every other row gets a fresh one.
        std::vector<entt::entity> nodeEntities(nodes.names.size());
        nodeEntities[0] = rootEntity;
        reg.create(nodeEntities.begin() + 1, nodeEntities.end());

        // Parents precede children in the table, so attaching in row order always finds the parent in place.
        for (size_t row = 0; row < nodes.names.size(); row++) {
            const entt::entity entity = nodeEntities[row];
            const entt::entity parentEntity = nodes.parents[row] == entt::null
                ? entt::entity{ entt::null }
            : nodeEntities[nodes.parents[row]];

            reg.emplace<roj::Transform>(entity, decomposeLocal(nodes.locals[row]));
            roj::Transform::attach(reg, entity, parentEntity);

            if (nodes.meshes[row] != entt::null) {
                reg.emplace<roj::GfxMeshInstance>(entity, model.meshes[nodes.meshes[row]]);
                if (nodes.skinned[row])
                    reg.emplace<roj::SkinPalette>(entity, roj::SkinPalette{ rootEntity, std::vector<glm::mat4>(model.joints.size(), glm::mat4(1.0f)) });
                if (nodes.materials[row] != entt::null)
                    reg.emplace<roj::GfxPbrMaterial>(entity, model.materials[nodes.materials[row]]);
            }
        }
        return nodeEntities;
    }

} // namespace

namespace roj {

    void instantiateModel(entt::handle root, const ModelScene& model) {
        if (model.nodes.names.empty())
            return;
        entt::registry& reg = *root.registry();

        std::vector<entt::entity> nodeEntities = instantiateNodes(reg, model, root.entity());

        if (model.animations.empty() && model.joints.empty())
            return; // static models don't pay for a rig

        std::vector<RigJoint> joints;
        joints.reserve(model.joints.size());
        for (const ModelJoint& joint : model.joints)
            joints.push_back({ joint.node, joint.inverseBindMatrix });

        reg.emplace<Rig>(root.entity(), makeRig(reg, std::move(nodeEntities), std::move(joints)));
    }

    ModelScene loadModel(entt::registry& reg, const std::string& path) {
        reg.ctx().emplace<tf::Executor>(std::thread::hardware_concurrency());
        resourceType<std::shared_ptr<bimg::ImageContainer>, std::filesystem::path>(reg).connect<&loadTextureFile>();
        resourceType<std::shared_ptr<bimg::ImageContainer>, aiTexture>(reg).connect<&readEmbeddedTexture>();
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, kImportFlags);

        ModelScene model;
        model.path = path;
        model.log += "Loading model: " + path + "\n";

        const bool importFailed = !scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode;
        if (importFailed) {
            model.log += importer.GetErrorString();
            return model;
        }

        model.meshes.resize(scene->mNumMeshes);
        model.materials.resize(scene->mNumMaterials);
        const NodeIndexMap nodeIndices = convertNodeTree(model, *scene);

        loadMeshes(model, reg, *scene, nodeIndices);
        loadAnimations(model, reg, *scene, nodeIndices);
        loadEmbeddedTextures(model, reg, *scene);   // before materials, which look textures up by name
        loadMaterials(model, reg, *scene);
        roj::resourcesFlush(reg);                   // assignNodeMaterials is gone


        return model;
    }

} // namespace roj