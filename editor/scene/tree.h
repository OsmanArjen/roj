// The entity tree (left pane): model update and drawing.
#pragma once
#include <entt/entity/fwd.hpp>
#include <editor/scene.h>

namespace roj::scene_ui {

	// Per-frame model update; call once before drawing, in this order:
	//   drop a dead selection, queue "reveal selection" if it changed from outside the tree,
	//   rebuild the parent/child edge list.
	void update_tree(SceneEditor& ed, const entt::registry& reg);

	// Header (title, entity count, Expand/Collapse) plus the scrolling node list.
	// Draws into the current window; the caller owns the surrounding child.
	void draw_tree_panel(SceneEditor& ed, const entt::registry& reg);

}
