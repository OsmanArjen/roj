#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>
#include <entt/entity/fwd.hpp>

namespace roj {

	enum class GfxVertexFormat {
		Int8,
		Uint8,
		Uint10,
		Int16,
		Uint16,
		Half,
		Float,
		Int32,
		Uint32,
	};

	enum class GfxVertexSemantic {
		Position,
		Normal,
		Tangent,
		Bitangent,
		Color0,
		Color1,
		Color2,
		Color3,
		Indices,
		Weight,
		TexCoord0,
		TexCoord1,
		TexCoord2,
		TexCoord3,
		TexCoord4,
		TexCoord5,
		TexCoord6,
		TexCoord7,
		TexCoord8,
		TexCoord9,
		TexCoord10,
		TexCoord11,
		TexCoord12,
		TexCoord13,
		TexCoord14,
		TexCoord15
	};

	struct GfxVertexAttribute {
		GfxVertexSemantic type;
		GfxVertexFormat format;
		uint8_t component;     // 1..4
		bool normalized;
	};

	// CPU-side mesh. Adding this component uploads it (GfxMeshData::on_construct);
	// patching it re-uploads; removing it frees the GPU buffers. The CPU copy is not
	// referenced after the upload, so it may be shrunk or cleared afterwards.
	//
	// Mapping notes (see mesh.cpp): the enums above are NOT bgfx's enums and are
	// translated explicitly. bgfx has no vertex attribute type for Int8 / Uint16 /
	// Int32 / Uint32 and only 8 texcoord slots, so those are rejected with an error
	// log instead of being cast into undefined territory.
	struct GfxMeshData {
		std::vector<GfxVertexAttribute> layout;
		std::vector<std::byte> vertices;
		std::vector<uint32_t> indices;

		static void on_construct(entt::registry&, entt::entity);
		static void on_update(entt::registry&, entt::entity);
		static void on_destroy(entt::registry&, entt::entity);
	};

	inline constexpr uint16_t kGfxInvalidIdx = UINT16_MAX;   // == bgfx::kInvalidHandle

	// Present on an entity only while its buffers exist; removed when they are freed.
	struct GfxMeshHandle {
		uint16_t vbh{ kGfxInvalidIdx };
		uint16_t ibh{ kGfxInvalidIdx };

		bool valid() const { return vbh != kGfxInvalidIdx && ibh != kGfxInvalidIdx; }
	};

	struct GfxMeshInstance { entt::entity id; };
}