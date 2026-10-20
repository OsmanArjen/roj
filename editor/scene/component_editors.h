// Inspector bodies, one per component type. component_table.cpp is the only caller.
//
// Two kinds of body:
//   edit:    bool body(ed, reg, entity, T& copy)          -> true if the copy was modified
//   inspect: void body(ed, const reg, entity, const T&)   (read-only)
// Edit bodies only ever touch a copy. The table's wrapper commits it through reg.patch, so the
// on_update signal always fires (your Transform::on_update depends on it) and no editor
// can forget to do it.
//
// Adding a component editor: write a body in the matching .cpp, declare it here, add one row
// in component_table.cpp.
#pragma once
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <editor/scene.h>
#include <editor/scene/hierarchy.h> // Node
#include <entity/transform.h>
#include <entity/camera.h>
#include <entity/animation.h>

namespace roj::scene_ui {

	// transform_editors.cpp
	bool transform_body(SceneEditor& ed, entt::registry& reg, entt::entity e, Transform& t);
	void node_body(SceneEditor& ed, const entt::registry& reg, entt::entity e, const Node& node);

	// camera_editors.cpp
	bool camera_body(SceneEditor& ed, entt::registry& reg, entt::entity e, Camera& c);
	bool fly_controller_body(SceneEditor& ed, entt::registry& reg, entt::entity e, FlyController& c);

	// animation_editors.cpp
	bool animation_player_body(SceneEditor& ed, entt::registry& reg, entt::entity e, AnimationPlayer& p);
	void rig_body(SceneEditor& ed, const entt::registry& reg, entt::entity e, const Rig& rig);
	void skin_palette_body(SceneEditor& ed, const entt::registry& reg, entt::entity e, const SkinPalette& skin);
	void clip_body(SceneEditor& ed, const entt::registry& reg, entt::entity e, const AnimationClip& clip);

}
