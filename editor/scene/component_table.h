// The registry of components the editor knows something about.
//
// One row per component. It drives the inspector sections (row order is display order),
// the badges in the tree and the inspector header, and the generic fallback list, which
// skips every type that owns a section or is marked hidden.
#pragma once
#include <vector>
#include <imgui.h>
#include <entt/core/fwd.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <editor/scene.h>
#include <editor/scene/widgets.h> // Badge

namespace roj::scene_ui {

	struct ComponentUi {
		entt::id_type type;
		const char* title;
		void (*draw)(SceneEditor&, entt::registry&, entt::entity); // nullptr: no section of its own
		Badge badge;                                               // text == nullptr: no badge
		bool hidden;                                               // keep out of the generic list
	};

	const std::vector<ComponentUi>& component_table();

	// Row for a component type hash, or nullptr if the editor knows nothing about it.
	const ComponentUi* find_ui(entt::id_type type);

	bool has_component(const entt::registry& reg, entt::id_type type, entt::entity e);

	// Draws the entity's badges left to right from pos; returns the width used.
	float draw_badges_at(const entt::registry& reg, entt::entity e, ImVec2 pos);

}
