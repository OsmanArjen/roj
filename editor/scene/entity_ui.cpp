#include <editor/scene/entity_ui.h>

#include <cstdio>

#include <imgui.h>
#include <entt/entity/registry.hpp>
#include <editor/scene/widgets.h>

namespace roj::scene_ui {

	void entity_label(const SceneEditor& ed, const entt::registry& reg, entt::entity e, char* out, std::size_t size) {
		const char* name = ed.name_of ? ed.name_of(reg, e) : nullptr;
		if (name && *name) {
			std::snprintf(out, size, "%s", name);
		}
		else {
			std::snprintf(out, size, "Entity %u", static_cast<unsigned>(entt::to_entity(e)));
		}
	}

	void entity_link(SceneEditor& ed, const entt::registry& reg, const char* name, entt::entity target) {
		ImGui::PushID(name);
		label_cell(name);

		if (target == entt::null) {
			ImGui::TextDisabled("null");
		}
		else if (!reg.valid(target)) {
			ImGui::TextDisabled("Entity %u (dead)", static_cast<unsigned>(entt::to_entity(target)));
		}
		else {
			char label[64];
			entity_label(ed, reg, target, label, sizeof label);
			if (link_button(label)) ed.selected = target;
		}
		ImGui::PopID();
	}

}
