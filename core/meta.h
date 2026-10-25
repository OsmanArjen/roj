#pragma once
#include <entt/core/type_traits.hpp>
#include <entt/core/type_info.hpp>
namespace roj
{

		template<auto Val>
		using constant = entt::integral_constant<Val>;

		template<auto... Vals>
		using constants = entt::type_list<constant<Vals>...>;

		template<typename... Cs>
		using constcat = entt::type_list_cat_t<Cs...>;

		template<auto V>
		using tag = entt::type_index<constant<V>>;

		template<std::size_t N, typename T, template<typename...> typename TL = entt::type_list, typename = std::make_index_sequence<N>>
		struct repeat;

		template<std::size_t N, typename T, template<typename...> typename TL, std::size_t... Is>
		struct repeat<N, T, TL, std::index_sequence<Is...>> {
			using type = TL<entt::unpack_as_type<T, entt::tag<Is>>...>;
		};

		template<std::size_t N, typename T, template<typename...> typename TL = entt::type_list>
		using repeat_t = typename repeat<N, T, TL>::type;
}