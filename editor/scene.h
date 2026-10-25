// scene_editor.h
#pragma once
#include <vector>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>

namespace roj {
	struct Camera;
	struct FlyController;

	// Editor state. The caller owns it, so there is no global state in the TU.
	struct SceneEditor {
		// Optional: how to display an entity's name. Return nullptr or "" for unnamed entities.
		const char* (*name_of)(const entt::registry&, entt::entity) = nullptr;

		entt::entity selected{ entt::null };

		// Rotation is edited as Euler degrees. The quaternion stays the source of truth.
		entt::entity eulerOwner{ entt::null };
		glm::vec3 euler{ 0.0f };
		glm::quat eulerSource{ 1.0f, 0.0f, 0.0f, 0.0f }; // quaternion as of the last sync/write

		// Reveal-in-tree: selection the tree was last synced to, and the ancestors to open.
		entt::entity revealed{ entt::null };
		std::vector<entt::entity> revealPath;
		bool scrollToSelected = false;

		// View state.
		float splitRatio = 0.38f;   // share of the window width given to the tree (left pane)
		enum class TreeCommand { None, ExpandAll, CollapseAll };
		TreeCommand treeCommand = TreeCommand::None; // applied for one frame

		// Per-frame scratch, kept as members to reuse allocations.
		struct Edge { entt::entity parent; entt::entity child; };
		std::vector<Edge> edges;                        // sorted by parent
		std::vector<entt::entity> roots;
		std::vector<entt::entity> crumbs;
	};

	void draw_scene_editor(SceneEditor& editor, entt::registry& reg);

	// Field editors for a Camera / FlyController that lives outside the registry.
	// Return true if anything changed.
	bool draw_camera_fields(Camera& camera);
	bool draw_fly_controller_fields(FlyController& controller);
}