#pragma once
#include "entt/entity/entity.hpp"
namespace roj
{	
	template<typename T, template<typename...> class Template>
	struct isSpecializationOf : std::false_type {};

	template<template<typename...> class Template, typename... Args>
	struct isSpecializationOf<Template<Args...>, Template> : std::true_type {};

	template<typename T, template<typename...> class U>
	concept isSpecialized = isSpecializationOf<T, U>::value;

	template<typename TList, template<typename> typename condition>
	struct filterTypes;

	template<typename VList, template<auto> typename condition>
	struct filterValues;

	template<template<typename...> class TList, template<typename> typename condition, typename... Ts>
	class filterTypes<TList<Ts...>, condition> {
		template<typename T>
		using keep = std::conditional_t<condition<T>::value, entt::type_list<T>, entt::type_list<>>;
	public:
		using type = entt::type_list_cat<keep<Ts>...>::type;
	};
	
	template<template<auto...> class VList, template<auto> typename condition, auto... Vs>
	class filterValues<VList<Vs...>, condition> {
		template<typename T>
		using keep = std::conditional_t<condition<T>::value, entt::value_list<T>, entt::value_list<>>;
	public:
		using type = entt::value_list_cat<keep<Vs>...>::type;
	};

	namespace internal
	{
		namespace type
		{
			template<auto target, typename values, typename = std::make_index_sequence<values::size>>
			struct find;

			template<auto target, auto... vals, std::size_t... Is>
			struct find<target, entt::value_list<vals...>, std::index_sequence<Is...>>
			{
				template<std::size_t Index, auto Target, auto Value>
				using match = std::conditional_t<
					(Value == Target),
					entt::value_list<Index>,
					entt::value_list<>
				>;
			
				using indices = entt::value_list_cat_t<match<Is, target, vals>...>;
			};

		}

	}

	namespace type
	{
		template<typename targets, typename source>
		struct find;

		template<auto... targets, auto... vals>
		struct find<entt::value_list<targets...>, entt::value_list<vals...> >
		{
			using indices = entt::type_list<typename internal::type::find<targets, entt::value_list<vals...>>::indices...>;
		};

		template<typename srcT>
		struct source;

		template<auto... content>
		struct source<entt::value_list<content...>>
		{
			using type = entt::value_list<content...>;
			

			template<std::size_t idx>
			static constexpr auto at = entt::value_list_element_v<idx,type>;
			
			template<std::size_t b>
			static constexpr auto at<entt::value_list<b>> = at<b>;
		};

		template<typename... content>
		struct source<entt::type_list<content...>>
		{
			using type = entt::type_list<content...>;
		};

		static int a = source<entt::value_list<1, 1, 2, 3, 4, 5, 6, 7, 8, 9>>::at<entt::value_list<0>>;
	}

	namespace internal
	{
		template<auto...>
		size_t nextTypeIndex()
		{
			static size_t index = 0;
			return index++;
		}
	}

	template<typename T, auto... Tag>
	struct typeIndex
	{
		static size_t value()
		{
			static size_t index = internal::nextTypeIndex<Tag...>();
			return index;
		}
	};

	template<typename TList, auto... Tag>
	struct typeIndexListOf;

	template<template<typename...> class TList, typename... Ts, auto... Tag>
	struct typeIndexListOf<TList<Ts...>, Tag...>: entt::type_list<typeIndex<Ts, Tag...>...> {};


}
