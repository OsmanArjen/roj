// The inspector (right pane): header, one section per known component, then the generic fallback.
#pragma once
#include <entt/entity/fwd.hpp>
#include <editor/scene.h>

namespace roj::scene_ui {

	// Draws into the current window; the caller owns the surrounding child.
	// Edits go through the component table, so they are committed with reg.patch.
	void draw_inspector(SceneEditor& ed, entt::registry& reg);

}
