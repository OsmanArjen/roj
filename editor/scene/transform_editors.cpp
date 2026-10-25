#include <editor/scene/component_editors.h>

#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <entt/entity/registry.hpp>
#include <editor/scene/entity_ui.h>
#include <editor/scene/widgets.h>

namespace roj::scene_ui {

	bool transform_body(SceneEditor& ed, entt::registry& reg, entt::entity e, Transform& t) {
		// Re-derive the Euler angles only if the quaternion changed behind our back
		// (different entity selected, animation, gameplay code).
		if (ed.eulerOwner != e || ed.eulerSource != t.rotation) {
			ed.euler = glm::degrees(glm::eulerAngles(t.rotation));
			ed.eulerSource = t.rotation;
			ed.eulerOwner = e;
		}

		bool edited = false;
		edited |= prop_vec3("Position", t.position, 0.01f);
		if (prop_vec3("Rotation (deg)", ed.euler, 0.5f)) {
			t.rotation = glm::quat(glm::radians(ed.euler));
			ed.eulerSource = t.rotation;
			edited = true;
		}
		edited |= prop_vec3("Scale", t.scale, 0.01f);

		if (const auto* world = reg.try_get<Transform::GlobalMatrix>(e)) {
			const glm::vec3 p((*world)[3]);
			ImGui::BeginDisabled();
			prop_value("World position", "%.3f   %.3f   %.3f", p.x, p.y, p.z);
			ImGui::EndDisabled();
		}
		return edited;
	}

	// Raw link fields, so the hierarchy encoding can be checked by eye.
	void node_body(SceneEditor& ed, const entt::registry& reg, entt::entity, const Node& node) {
		entity_link(ed, reg, "source", node.source);
		entity_link(ed, reg, "target", node.target);
		entity_link(ed, reg, "successor", node.successor);
		entity_link(ed, reg, "predecessor", node.predecessor);
		prop_value("lineage", "%u", static_cast<unsigned>(node.lineage));
		prop_value("depth", "%u", static_cast<unsigned>(node.depth));
	}

}
