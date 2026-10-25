#include <entity/transform.h>
#include <entt/entity/registry.hpp>
namespace roj
{
	Transform::operator glm::mat4() const {
		return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
	}

	void Transform::on_construct(entt::registry& reg, entt::entity entity) {
		reg.emplace<GlobalMatrix>(entity);
		reg.emplace<Transform::Node>(entity);
		reg.emplace<Transform::ChangeFlag>(entity);
	}

	void Transform::on_update(entt::registry& reg, entt::entity entity) {
		auto& nodes = reg.storage<Transform::Node>();
		auto& dirty = reg.storage<Transform::ChangeFlag>();
		entt::entity it = entity;
		bool traversal = true;
		while (traversal) {
			if (dirty.contains(it)) break;
			dirty.emplace(it);
			auto& itn = nodes.get(it);
			if (itn.target != entt::null) {
				it = itn.target;
			}
			else {
				entt::entity next = itn.successor;
				while (next == entt::null) {
					if (it == entity) {
						traversal = false;
						break;
					}
					it = nodes.get(it).source;
					next = nodes.get(it).successor;

				}

				if (it == entity) {
					traversal = false;
					break;
				}
				it = next;
			}

		}

	}

	void Transform::attach(entt::registry& reg, entt::entity child, entt::entity parent) {
		auto& node = reg.get<Transform::Node>(child);

		if (node.predecessor != entt::null) {
			reg.get<Transform::Node>(node.predecessor).successor = node.successor;
		}
		else if (node.source != entt::null) {
			auto& oldParent = reg.get<Transform::Node>(node.source);
			if (oldParent.target == child)
				oldParent.target = node.successor;
		}
		if (node.successor != entt::null) {
			reg.get<Transform::Node>(node.successor).predecessor = node.predecessor;
		}

		node.source = parent;
		node.predecessor = entt::null;
		node.successor = entt::null;

		if (parent != entt::null) {
			auto& parentNode = reg.get<Transform::Node>(parent);
			node.lineage = parentNode.lineage;
			node.depth = parentNode.depth + 1;

			if (parentNode.target == entt::null) {
				parentNode.target = child;
			}
			else {
				entt::entity last = parentNode.target;
				while (reg.get<Transform::Node>(last).successor != entt::null)
					last = reg.get<Transform::Node>(last).successor;
				reg.get<Transform::Node>(last).successor = child;
				node.predecessor = last;
			}
		}
		else {
			node.depth = 0;
		}

		reg.patch<Transform>(child); 
	}

	void Transform::Node::on_construct(entt::registry& reg, entt::entity entity) {
		static uint16_t nTree = 0;
		auto& node = reg.get<Transform::Node>(entity);
		Transform::Node* parent = reg.try_get<Transform::Node>(node.source);
		node.lineage = parent ? parent->lineage : nTree++;
		node.depth = parent ? parent->depth + 1 : 0;
	}

	void Transform::propagate(entt::registry& reg){
		reg.sort<Transform::ChangeFlag>([&reg](const entt::entity lhs, const entt::entity rhs) {
			auto& lnode = reg.get<Transform::Node>(lhs);
			auto& rnode = reg.get<Transform::Node>(rhs);

			return (lnode.lineage < rnode.lineage) ||
				((lnode.lineage == rnode.lineage) && (lnode.depth < rnode.depth));
			});

		for (auto&& entity : reg.view<Transform::ChangeFlag>()) {
			auto& node = reg.get<Transform::Node>(entity);
			auto& local = reg.get<Transform>(entity);
			auto& global = reg.get<Transform::GlobalMatrix>(entity);

			if (node.source != entt::null) {
				auto& parentGlobal = reg.get<Transform::GlobalMatrix>(node.source);
				global = static_cast<Transform::GlobalMatrix>(glm::mat4(parentGlobal) * glm::mat4(local));
			}
			else {
				global = static_cast<Transform::GlobalMatrix>(glm::mat4(local));
			}

		}

		reg.reset(entt::type_hash<Transform::ChangeFlag>::value());
	}
}