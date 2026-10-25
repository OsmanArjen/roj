#include <editor/scene/hierarchy.h>

#include <entt/entity/registry.hpp>

namespace roj::scene_ui {

	entt::entity parent_of(const entt::registry& reg, entt::entity self, const Node& node) {
		const entt::entity p = node.source;
		if (p == entt::null || p == self || !reg.valid(p) || !reg.all_of<Transform>(p)) {
			return entt::null; // dangling or missing parent: show as a root rather than hide it
		}
		return p;
	}

	// The bound only guards against a corrupted (cyclic) hierarchy.
	void collect_ancestors(const entt::registry& reg, entt::entity e, std::vector<entt::entity>& out) {
		out.clear();
		for (int guard = 0; guard < 65536 && e != entt::null; ++guard) {
			const Node* node = reg.try_get<Node>(e);
			e = node ? parent_of(reg, e, *node) : entt::entity{ entt::null };
			if (e != entt::null) out.push_back(e);
		}
	}

}
