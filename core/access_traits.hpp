#pragma once
#include <functional>
#include <memory>
#include "entt/entt.hpp"
#include "utils.hpp"
namespace roj
{

	template<typename... setTs>
	struct AccessPattern;

	template<auto AccessTag>
	struct AccessPolicy
	{
		template<typename T>
		struct apply : std::false_type {};
	};

	template<typename Ts, auto tag>
	struct AccessSet
	{
		using types = filterTypes<Ts, AccessPolicy<tag>::apply>::type;
		using build = AccessPattern<AccessSet<Ts, tag>>;

		template<typename newTs, auto newTag> requires (newTag != tag)
		using chain = AccessPattern<AccessSet<Ts, tag>, AccessSet<newTs, newTag>>;
	};

	namespace internal
	{

		struct AccessPattern
		{


			template<typename>
			struct process;

			template<typename... setTypes, auto... setTags>
			struct process<roj::AccessPattern<AccessSet<setTypes, setTags>...>> {
				using uniqueTags = entt::value_list_unique_t<entt::value_list<setTags...>>;
				using tagMap = typename roj::type::find<uniqueTags, entt::value_list<setTags...>>::indices;

			};

			using b = process<roj::AccessPattern<AccessSet<entt::type_list<int>, 0>, AccessSet<entt::type_list<float>, 1>, AccessSet<entt::type_list<char>, 0>>>;
		};

	}

	template<typename... TList, auto... Tags>
	struct AccessPattern<AccessSet<TList, Tags>...>
	{
		using tags = internal::AccessPattern::process<AccessPattern>::uniqueTags;
		using sets = internal::AccessPattern::process<AccessPattern>::list;
		
		template<typename newTs, auto newTag>
		using chain = AccessPattern<AccessSet<TList, Tags>..., AccessSet<newTs, newTag>>;

		template<typename... patterns>
		using pipe = entt::type_list_transform_t<entt::type_list_cat_t<sets, typename patterns::sets...>, AccessPattern>;
	};
}