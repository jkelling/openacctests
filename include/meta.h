#pragma once

#include <utility>

#define BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION

namespace meta
{
	namespace detail
	{
		//#############################################################################
		//! Empty dependent type.
		template<
			typename T>
		struct Empty
		{};

		//#############################################################################
		template<
			typename... Ts>
		struct IsParameterPackSetImpl;
		//#############################################################################
		template<>
		struct IsParameterPackSetImpl<>
		{
			static constexpr bool value = true;
		};
		//#############################################################################
		// Based on code by Roland Bock: https://gist.github.com/rbock/ad8eedde80c060132a18
		// Linearly inherits from empty<T> and checks if it has already inherited from this type.
		template<
			typename T,
			typename... Ts>
		struct IsParameterPackSetImpl<T, Ts...> :
			public IsParameterPackSetImpl<Ts...>,
			public virtual Empty<T>
		{
			using Base = IsParameterPackSetImpl<Ts...>;

			static constexpr bool value = Base::value && !std::is_base_of<Empty<T>, Base>::value;
		};
	}
	//#############################################################################
	//! Trait that tells if the parameter pack contains only unique (no equal) types.
	template<
		typename... Ts>
	using IsParameterPackSet = detail::IsParameterPackSetImpl<Ts...>;

	namespace detail
	{
		//#############################################################################
		template<
			typename TList>
		struct IsSetImpl;
		//#############################################################################
		template<
			template<typename...> class TList,
			typename... Ts>
		struct IsSetImpl<
			TList<Ts...>>
		{
			static constexpr bool value = IsParameterPackSet<Ts...>::value;
		};
	}
	//#############################################################################
	//! Trait that tells if the template contains only unique (no equal) types.
	template<
		typename TList>
	using IsSet = detail::IsSetImpl<TList>;
// }
// 
// namespace meta
// {
	//#############################################################################
	// This could be replaced with c++14 std::IntegerSequence if we raise the minimum.
	template<
		typename T,
		T... Tvals>
	struct IntegerSequence
	{
		static_assert(std::is_integral<T>::value, "IntegerSequence<T, I...> requires T to be an integral type.");

		using type = IntegerSequence<T, Tvals...>;
		using value_type = T;

		static auto size() noexcept
		-> std::size_t
		{
			return (sizeof...(Tvals));
		}
	};

	namespace detail
	{
		//#############################################################################
		template<
			typename TDstType,
			typename TIntegerSequence>
		struct ConvertIntegerSequence;
		//#############################################################################
		template<
			typename TDstType,
			typename T,
			T... Tvals>
		struct ConvertIntegerSequence<
			TDstType,
			IntegerSequence<T, Tvals...>>
		{
			using type = IntegerSequence<TDstType, static_cast<TDstType>(Tvals)...>;
		};
	}
	//#############################################################################
	template<
		typename TDstType,
		typename TIntegerSequence>
	using ConvertIntegerSequence = typename detail::ConvertIntegerSequence<TDstType, TIntegerSequence>::type;

	namespace detail
	{
		//#############################################################################
		template<
			template<typename...> class TList,
			typename T,
			template<T> class TOp,
			typename TIntegerSequence>
		struct TransformIntegerSequence;
		//#############################################################################
		template<
			template<typename...> class TList,
			typename T,
			template<T> class TOp,
			T... Tvals>
		struct TransformIntegerSequence<
			TList,
			T,
			TOp,
			IntegerSequence<T, Tvals...>>
		{
			using type =
				TList<
					TOp<Tvals>...>;
		};
	}
	//#############################################################################
	template<
		template<typename...> class TList,
		typename T,
		template<T> class TOp,
		typename TIntegerSequence>
	using TransformIntegerSequence = typename detail::TransformIntegerSequence<TList, T, TOp, TIntegerSequence>::type;

	namespace detail
	{
		//#############################################################################
		template<bool TisSizeNegative, bool TbIsBegin, typename T, T Tbegin, typename TIntCon, typename TIntSeq>
		struct MakeIntegerSequenceHelper
		{
			static_assert(!TisSizeNegative, "MakeIntegerSequence<T, N> requires N to be non-negative.");
		};
		//#############################################################################
		template<typename T, T Tbegin, T... Tvals>
		struct MakeIntegerSequenceHelper<false, true, T, Tbegin, std::integral_constant<T, Tbegin>, IntegerSequence<T, Tvals...> > :
			IntegerSequence<T, Tvals...>
		{};
		//#############################################################################
		template<typename T, T Tbegin, T TIdx, T... Tvals>
		struct MakeIntegerSequenceHelper<false, false, T, Tbegin, std::integral_constant<T, TIdx>, IntegerSequence<T, Tvals...> > :
			MakeIntegerSequenceHelper<false, TIdx == (Tbegin+1), T, Tbegin, std::integral_constant<T, TIdx - 1>, IntegerSequence<T, TIdx - 1, Tvals...> >
		{};
	}

	//#############################################################################
	template<typename T, T Tbegin, T Tsize>
	using MakeIntegerSequenceOffset = typename detail::MakeIntegerSequenceHelper<(Tsize < 0), (Tsize == 0), T, Tbegin, std::integral_constant<T, Tbegin+Tsize>, IntegerSequence<T> >::type;

	//#############################################################################
	template<typename T, T Tsize>
	using MakeIntegerSequence = MakeIntegerSequenceOffset<T, 0u, Tsize>;


	//#############################################################################
	template<
		std::size_t... Tvals>
	using IndexSequence = IntegerSequence<std::size_t, Tvals...>;

	//#############################################################################
	template<
		typename T,
		T Tbegin,
		T Tsize>
	using MakeIndexSequenceOffset = MakeIntegerSequenceOffset<std::size_t, Tbegin, Tsize>;

	//#############################################################################
	template<
		std::size_t Tsize>
	using MakeIndexSequence = MakeIntegerSequence<std::size_t, Tsize>;

	//#############################################################################
	template<
		typename... Ts>
	using IndexSequenceFor = MakeIndexSequence<sizeof...(Ts)>;


	//#############################################################################
	//! Checks if the integral values are unique.
	template<
		typename T,
		T... Tvals>
	struct IntegralValuesUnique
	{
		static constexpr bool value = meta::IsParameterPackSet<std::integral_constant<T, Tvals>...>::value;
	};

	//#############################################################################
	//! Checks if the values in the index sequence are unique.
	template<
		typename TIntegerSequence>
	struct IntegerSequenceValuesUnique;
	//#############################################################################
	//! Checks if the values in the index sequence are unique.
	template<
		typename T,
		T... Tvals>
	struct IntegerSequenceValuesUnique<
		IntegerSequence<T, Tvals...>>
	{
		static constexpr bool value = IntegralValuesUnique<T, Tvals...>::value;
	};

	//#############################################################################
	//! Checks if the integral values are within the given range.
	template<
		typename T,
		T Tmin,
		T Tmax,
		T... Tvals>
	struct IntegralValuesInRange;
	//#############################################################################
	//! Checks if the integral values are within the given range.
	template<
		typename T,
		T Tmin,
		T Tmax>
	struct IntegralValuesInRange<
		T,
		Tmin,
		Tmax>
	{
		static constexpr bool value = true;
	};
	//#############################################################################
	//! Checks if the integral values are within the given range.
	template<
		typename T,
		T Tmin,
		T Tmax,
		T I,
		T... Tvals>
	struct IntegralValuesInRange<
		T,
		Tmin,
		Tmax,
		I,
		Tvals...>
	{
		static constexpr bool value = (I >= Tmin) && (I <=Tmax) && IntegralValuesInRange<T, Tmin, Tmax, Tvals...>::value;
	};

	//#############################################################################
	//! Checks if the values in the index sequence are within the given range.
	template<
		typename TIntegerSequence,
		typename T,
		T Tmin,
		T Tmax>
	struct IntegerSequenceValuesInRange;
	//#############################################################################
	//! Checks if the values in the index sequence are within the given range.
	template<
		typename T,
		T... Tvals,
		T Tmin,
		T Tmax>
	struct IntegerSequenceValuesInRange<
		IntegerSequence<T, Tvals...>,
		T,
		Tmin,
		Tmax>
	{
		static constexpr bool value = IntegralValuesInRange<T, Tmin, Tmax, Tvals...>::value;
	};
// }
// 
// namespace meta
// {
	//-----------------------------------------------------------------------------
	// C++17 std::invoke
	namespace detail
	{
		template<class F, class... Args>
		inline auto invoke_impl(F && f, Args &&... args)
		-> decltype(std::forward<F>(f)(std::forward<Args>(args)...))
		{
			return std::forward<F>(f)(std::forward<Args>(args)...);
		}

		template<class Base, class T, class Derived>
		inline auto invoke_impl(T Base::*pmd, Derived && ref)
		-> decltype(std::forward<Derived>(ref).*pmd)
		{
			return std::forward<Derived>(ref).*pmd;
		}

		template<class PMD, class Pointer>
		inline auto invoke_impl(PMD pmd, Pointer && ptr)
		-> decltype((*std::forward<Pointer>(ptr)).*pmd)
		{
			return (*std::forward<Pointer>(ptr)).*pmd;
		}

		template<class Base, class T, class Derived, class... Args>
		inline auto invoke_impl(T Base::*pmf, Derived && ref, Args &&... args)
		-> decltype((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...))
		{
			return (std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...);
		}

		template<class PMF, class Pointer, class... Args>
		inline auto invoke_impl(PMF pmf, Pointer && ptr, Args &&... args)
		-> decltype(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...))
		{
			return ((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...);
		}
	}

	template< class F, class... ArgTypes>
	auto invoke(F && f, ArgTypes &&... args)
#ifdef BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION
	-> decltype(detail::invoke_impl(std::forward<F>(f), std::forward<ArgTypes>(args)...))
#endif
	{
		return detail::invoke_impl(std::forward<F>(f), std::forward<ArgTypes>(args)...);
	}

	//-----------------------------------------------------------------------------
	// C++17 std::apply
	namespace detail
	{
		template<class F, class Tuple, std::size_t... I>
		auto apply_impl( F && f, Tuple &&t, meta::IndexSequence<I...> )
#ifdef BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION
		-> decltype(
			meta::invoke(
				std::forward<F>(f),
				std::get<I>(std::forward<Tuple>(t))...))
#endif
		{
			// If the the index sequence is empty, t will not be used at all.

			return
				meta::invoke(
					std::forward<F>(f),
					std::get<I>(std::forward<Tuple>(t))...);
		}
	}

	template<class F, class Tuple>
	auto apply(F && f, Tuple && t)
#ifdef BOOST_NO_CXX14_RETURN_TYPE_DEDUCTION
	-> decltype(
		detail::apply_impl(
			std::forward<F>(f),
			std::forward<Tuple>(t),
			meta::MakeIndexSequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{}))
#endif
	{
		return
			detail::apply_impl(
				std::forward<F>(f),
				std::forward<Tuple>(t),
				meta::MakeIndexSequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{});
	}
}
