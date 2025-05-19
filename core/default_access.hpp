#pragma once
#include "access_traits.hpp"

namespace roj
{
	enum DefaultAccessed
	{
		ContextTag,
		ComponentTag
	};

	struct AccessPolicy<ContextTag>
	{
		template<typename T>
		struct apply : std::bool_constant<(std::is_lvalue_reference_v<T> &&
			!std::is_const_v<std::remove_reference_t<T>>)>{};
	};

	struct AccessPolicy<ComponentTag>
	{
		template<typename T>
		struct apply : std::bool_constant<(!std::is_const_v<T>)> {};
	};

	template<typename T>
	struct Access
	{
		using type = T&;
		using context = T;
		using pattern = AccessSet<entt::type_list<T>, ContextTag>::build;
		static type get(context& ctx) noexcept { return ctx; }
	};

	template<typename T>
	struct Access<const T> : Access<T> { using type = const typename Access<T>::type; };

	template<typename... Get, typename... Exclude>
	struct Access< entt::basic_view<entt::get_t<Get...>, entt::exclude_t<Exclude...>>>
	{
		using type = entt::basic_view<entt::get_t<Get...>, entt::exclude_t<Exclude...>>;
		using context = entt::registry;
		using pattern = AccessSet<entt::type_list<typename Get::element_type...>, ComponentTag>::build;
		static type get(context& ctx) noexcept { return entt::as_view( ctx ); }
	};
	
	template<typename T>
	concept Accessable = requires(typename Access<T>::context & ctx) {
		typename Access<T>::type;
		typename Access<T>::context;
		typename Access<T>::pattern;
		requires(roj::isSpecialized<typename Access<T>::pattern, AccessPattern>);
		{ Access<T>::get(ctx) } -> std::convertible_to<typename Access<T>::type>;
	};
	
}

/*


TODO:
-AccessSet: chain
-AccessPattern: chain, pipe
{same Accesstaged AccessSets are merged, different ones are chained}









*/

