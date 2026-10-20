// How the scene hierarchy is encoded in the registry. No ImGui, no editor state:
// if Transform::Node ever changes meaning, this is the only place to touch.
#pragma once
#include <vector>
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <entity/transform.h>

namespace roj::scene_ui {

	using Node = Transform::Node;

	// The only function that knows how Transform::Node encodes the parent link.
	// Returns entt::null for roots, and for dangling or missing parents (shown as roots rather than hidden).
	entt::entity parent_of(const entt::registry& reg, entt::entity self, const Node& node);

	// Ancestors of e, nearest first. Guarded against a corrupted (cyclic) hierarchy.
	void collect_ancestors(const entt::registry& reg, entt::entity e, std::vector<entt::entity>& out);

}
