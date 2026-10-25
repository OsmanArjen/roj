// How an entity is shown to the user: its name, and a clickable reference to it.
#pragma once
#include <cstddef>
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <editor/scene.h>

namespace roj::scene_ui {

	// The one place that decides how an entity is named (SceneEditor::name_of, else "Entity <id>").
	// e must be valid.
	void entity_label(const SceneEditor& ed, const entt::registry& reg, entt::entity e, char* out, std::size_t size);

	// Entity-valued field: a labelled row whose value is a link that selects that entity.
	void entity_link(SceneEditor& ed, const entt::registry& reg, const char* name, entt::entity target);

}
