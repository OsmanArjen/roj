#pragma once  
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/quaternion.hpp"
#include "entt/entity/fwd.hpp"
#include "entt/entity/entity.hpp"

namespace roj {  
	struct Transform {
		struct Node;
		struct ChangeFlag {};
		struct GlobalMatrix : glm::mat4 {};
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;

		operator glm::mat4() const;
		static void on_construct(entt::registry& reg, entt::entity entity);
		static void on_update(entt::registry& reg, entt::entity entity);
		static void propagate(entt::registry& reg);
		static void attach(entt::registry& reg, entt::entity child, entt::entity parent);
	};

	struct Transform::Node {

		entt::entity source{ entt::null };
		entt::entity target{ entt::null };

		entt::entity successor{ entt::null };
		entt::entity predecessor{ entt::null };

		uint16_t lineage;
		uint16_t depth;

		static void on_construct(entt::registry& reg, entt::entity entity);
	};

	

}