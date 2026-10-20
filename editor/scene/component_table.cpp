#include <editor/scene/component_table.h>

#include <entt/entity/registry.hpp>
#include <editor/scene/component_editors.h>

namespace roj::scene_ui {

	namespace {

		template <typename T, auto Draw>
		void edit_component(SceneEditor& ed, entt::registry& reg, entt::entity e) {
			T value = reg.get<T>(e);
			if (Draw(ed, reg, e, value)) {
				reg.patch<T>(e, [&](T& dst) { dst = value; });
			}
		}

		template <typename T, auto Draw>
		void inspect_component(SceneEditor& ed, entt::registry& reg, entt::entity e) {
			const entt::registry& creg = reg;
			Draw(ed, creg, e, creg.get<T>(e));
		}

		template <typename T, auto Draw>
		ComponentUi edit_row(const char* title, Badge badge = {}) {
			return { entt::type_hash<T>::value(), title, &edit_component<T, Draw>, badge, false };
		}

		template <typename T, auto Draw>
		ComponentUi inspect_row(const char* title, Badge badge = {}) {
			return { entt::type_hash<T>::value(), title, &inspect_component<T, Draw>, badge, false };
		}

		// Component without an editor that should still get a badge in the tree.
		template <typename T>
		ComponentUi badge_row(Badge badge) { return { entt::type_hash<T>::value(), nullptr, nullptr, badge, false }; }

		template <typename T>
		ComponentUi hidden_row() { return { entt::type_hash<T>::value(), nullptr, nullptr, {}, true }; }

	} // namespace

	// ---- add a component editor: write a body (component_editors.h), add one row here ----------
	const std::vector<ComponentUi>& component_table() {
		static const std::vector<ComponentUi> table{
			edit_row<Transform, transform_body>("Transform"),
			inspect_row<Node, node_body>("Transform::Node"),
			hidden_row<Transform::GlobalMatrix>(), // summarised inside the Transform section
			edit_row<Camera, camera_body>("Camera", { "CAM", IM_COL32(190, 120, 40, 255) }),
			edit_row<FlyController, fly_controller_body>("FlyController"),
			edit_row<AnimationPlayer, animation_player_body>("AnimationPlayer", { "ANIM", IM_COL32(60, 150, 90, 255) }),
			inspect_row<Rig, rig_body>("Rig", { "RIG", IM_COL32(130, 90, 180, 255) }),
			inspect_row<SkinPalette, skin_palette_body>("SkinPalette", { "SKIN", IM_COL32(50, 140, 160, 255) }),
			inspect_row<AnimationClip, clip_body>("AnimationClip", { "CLIP", IM_COL32(170, 150, 50, 255) }),
		};
		return table;
	}

	const ComponentUi* find_ui(entt::id_type type) {
		for (const ComponentUi& row : component_table()) {
			if (row.type == type) return &row;
		}
		return nullptr;
	}

	bool has_component(const entt::registry& reg, entt::id_type type, entt::entity e) {
		const auto* storage = reg.storage(type);
		return storage && storage->contains(e);
	}

	float draw_badges_at(const entt::registry& reg, entt::entity e, ImVec2 pos) {
		ImDrawList* dl = ImGui::GetWindowDrawList();
		float used = 0.0f;
		for (const ComponentUi& row : component_table()) {
			if (!row.badge.text || !has_component(reg, row.type, e)) continue;
			const float width = draw_badge(dl, ImVec2(pos.x + used, pos.y), row.badge);
			used += width + 4.0f;
		}
		return used;
	}

}
