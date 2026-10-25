#pragma once
#include "meta.h"
#include <entt/container/dense_set.hpp>
#include <entt/graph/adjacency_matrix.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/helper.hpp>
#include <taskflow/taskflow.hpp>

namespace roj
{
	namespace internal
	{
		template<typename Type>
		struct access {
			using ro = std::conditional_t<
				std::is_const_v<Type>,
				entt::type_list<std::remove_const_t<Type>>,
				entt::type_list<>>;

			using rw = std::conditional_t<
				!std::is_const_v<Type>,
				entt::type_list<Type>,
				entt::type_list<>>;

			static decltype(auto) get(entt::registry& reg) {
				if constexpr(std::is_same_v<std::remove_const_t<Type>, entt::registry>) {
					return reg;
				} else {
					return reg.ctx().template emplace<std::remove_reference_t<Type>>();
				}
			}
		};

		template<typename... Get, typename... Exclude>
		struct access<entt::basic_view<entt::get_t<Get...>, entt::exclude_t<Exclude...>>> {
			using type = entt::basic_view<entt::get_t<Get...>, entt::exclude_t<Exclude...>>;
			using ro = entt::type_list_cat_t<entt::type_list<typename Exclude::element_type...>, typename access<entt::constness_as_t<typename Get::element_type, Get>>::ro...>;
			using rw = entt::type_list_cat_t<typename access<entt::constness_as_t<typename Get::element_type, Get>>::rw...>;
			static decltype(auto) get(entt::registry& reg) {
				return static_cast<type>(entt::as_view( reg ));
			}
		};

		template<typename... Get, typename... Exclude>
		struct access<const entt::basic_view<entt::get_t<Get...>, entt::exclude_t<Exclude...>>>
			: access<entt::basic_view<entt::get_t<Get...>, entt::exclude_t<Exclude...>>> {
		};

		template<typename... Owned, typename... Get, typename... Exclude>
		struct access<entt::basic_group<entt::owned_t<Owned...>, entt::get_t<Get...>, entt::exclude_t<Exclude...>>> {
			using type = entt::basic_group<entt::owned_t<Owned...>, entt::get_t<Get...>, entt::exclude_t<Exclude...>>;
			using ro = entt::type_list_cat_t<entt::type_list<typename Exclude::element_type...>, typename access<entt::constness_as_t<typename Get::element_type, Get>>::ro..., typename access<entt::constness_as_t<typename Owned::element_type, Owned>>::ro...>;
			using rw = entt::type_list_cat_t<typename access<entt::constness_as_t<typename Get::element_type, Get>>::rw..., typename access<entt::constness_as_t<typename Owned::element_type, Owned>>::rw...>;
			static decltype(auto) get(entt::registry& reg) {
				return static_cast<type>(entt::as_group( reg ));
			}
		};

		template<typename... Owned, typename... Get, typename... Exclude>
		struct access<const entt::basic_group<entt::owned_t<Owned...>, entt::get_t<Get...>, entt::exclude_t<Exclude...>>>
			: access<entt::basic_group<entt::owned_t<Owned...>, entt::get_t<Get...>, entt::exclude_t<Exclude...>>> {
		};

		template<typename... Args>
		struct access<entt::type_list<Args...>> {
			using args = entt::type_list<std::remove_const_t<Args>...>;
			using ro = entt::type_list_cat_t<typename access<Args>::ro...>;
			using rw = entt::type_list_cat_t<typename access<Args>::rw...>;

			static auto get(entt::registry& reg) {
				return std::tuple<decltype(access<Args>::get(reg))...>(access<Args>::get(reg)...);
			}
		};

		template<typename Ret, typename... Args>
		access<entt::type_list<std::remove_reference_t<Args>...>> declaccess(Ret(*)(Args...)) {return {};}
	}

	template<auto Fn>
	struct access {
		using type = decltype(internal::declaccess(Fn));
		using ro = typename type::ro;
		using rw = typename type::rw;
		static decltype(auto) get(entt::registry& reg) {
			return type::get(reg);
		}
	};

	class SystemFlow {
		using access_type = std::pair<std::size_t, bool>;
		using edge_type = std::pair<std::size_t, std::size_t>;

		using resource_map = entt::dense_map<entt::id_type, std::vector<access_type>, std::identity>;
		using resource_set = entt::dense_set<entt::entity, std::identity>;
		using system_set = entt::dense_set<std::size_t, std::identity>;
		using edge_list = std::vector<edge_type>;
	public:
		using matrix_type = entt::adjacency_matrix<entt::directed_tag>;
		using callback_t = void(*)(entt::registry&);

		template<auto Fn>
		void emplace() {
			std::size_t target = vertex<Fn>();
			if (!m_callbacks[target]) {
				setAccess(target, typename roj::access<Fn>::ro{}, typename roj::access<Fn>::rw{});
				m_callbacks[target] = +[](entt::registry& reg) {
					std::apply(Fn, roj::access<Fn>::get(reg));
				};
			}
		}
		
		template<auto PredFn, auto SuccFn>
		void insert() {
			m_dependencies.emplace_back(vertex<PredFn>(), vertex<SuccFn>());
		}

		bool dense() const {
			return std::all_of(m_callbacks.cbegin(), m_callbacks.cend(), [](callback_t ptr)-> bool {return ptr; });
		}

		const std::vector<callback_t>& vertices() const {
			return m_callbacks;
		}
		
		matrix_type graph() const {
			matrix_type matrix(m_systems.size());
			setImplicitEdges(matrix);
			transitiveClosure(matrix);
			setExplicitEdges(matrix);
			transitiveReduction(matrix);
			return matrix;
		}

		tf::Taskflow build(entt::registry& reg) const {
			const auto matrix = graph();
			const auto& callbacks = vertices();
			tf::Taskflow taskflow;
			std::vector<tf::Task> tasks;
			tasks.reserve(callbacks.size());
			taskflow.clear();
			for (std::size_t v : matrix.vertices()) {
				if (callbacks[v]) {
					tasks.emplace_back(
						taskflow.emplace([systemfn = callbacks[v], &registry = reg]() {
							systemfn(registry);
							})
					);
				}
				else {
					tasks.emplace_back(taskflow.placeholder());
				}
			}

			for (auto [pred, succ] : matrix.edges()) {
				tasks[pred].precede(tasks[succ]);
			}

			return taskflow;
		}

	private:
		void transitiveReduction(matrix_type& graph) const {
			const auto length = graph.size();
			for (std::size_t v{}; v < length; ++v) {
				graph.erase(v, v);
			}
			for (std::size_t vj{}; vj < length; ++vj) {
				for (std::size_t vi{}; vi < length; ++vi) {
					if (graph.contains(vi, vj)) {
						for (std::size_t vk{}; vk < length; ++vk) {
							if (graph.contains(vj, vk)) {
								graph.erase(vi, vk);
							}
						}
					}
				}
			}
		}
		void transitiveClosure(matrix_type& graph) const {
			const auto length = graph.size();
			for (std::size_t v{}; v < length; ++v) {
				graph.insert(v, v);
			}

			for (std::size_t vk{}; vk < length; ++vk) {
				for (std::size_t vi{}; vi < length; ++vi) {
					for (std::size_t vj{}; vj < length; ++vj) {
						if (graph.contains(vi, vk) && graph.contains(vk, vj)) {
							graph.insert(vi, vj);
						}
					}
				}
			}
		}
		void setImplicitEdges(matrix_type& matrix) const {
			for (const auto& elem : m_accesses) {
				const auto last = elem.second.cend();
				auto it = elem.second.cbegin();

				while (it != last) {
					if (it->second) {
						// rw item
						if (auto curr = it++; it != last) {
							if (it->second) {
								matrix.insert(curr->first, it->first);
							}
							else if (const auto next = std::find_if(it, last, [](const auto& value) { return value.second; }); next != last) {
								for (; it != next; ++it) {
									matrix.insert(curr->first, it->first);
									matrix.insert(it->first, next->first);
								}
							}
							else {
								for (; it != next; ++it) {
									matrix.insert(curr->first, it->first);
								}
							}
						}
					}
					else {
						// ro item (first iteration only)
						if (const auto next = std::find_if(it, last, [](const auto& value) { return value.second; }); next != last) {
							for (; it != next; ++it) {
								matrix.insert(it->first, next->first);
							}
						}
						else {
							it = last;
						}
					}
				}
			}
		}
		void setExplicitEdges(matrix_type& graph) const {
			for (auto [lv, rv] : m_dependencies) {
				if (graph.contains(lv, rv)) continue;
				for (auto [pred, _] : graph.in_edges(lv)) {
					if (graph.contains(rv, pred)) {
						graph.erase(pred, lv);
						graph.insert(lv, pred);
					}
				}
				graph.insert(lv, rv);
			}
		}

		template<typename... ro, typename... rw>
		void setAccess(std::size_t target, entt::type_list<ro...>, entt::type_list<rw...>) {
			m_accesses[entt::type_hash<entt::registry>::value()].emplace_back(target, false);
			(m_accesses[entt::type_hash<ro>::value()].emplace_back(target, false), ...);
			(m_accesses[entt::type_hash<rw>::value()].emplace_back(target, true), ...);
		}

		template<auto Fn>
		std::size_t vertex() {
			auto [target, insertion] = m_systems.emplace(roj::tag<Fn>::value());
			size_t idx = static_cast<std::size_t>(target - m_systems.begin());
			m_callbacks.resize((insertion) ? m_callbacks.size() + 1 : m_callbacks.size());
			return idx;
		}
	private:
		system_set m_systems;
		resource_map m_accesses;
		edge_list m_dependencies;
		std::vector<callback_t> m_callbacks;	
	};


}
