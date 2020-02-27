#pragma once
namespace std
{
	template<typename F>
	auto apply(F &&f)
	{
		return f();
	}

	template<typename F,
		typename A0>
	auto apply(F &&f, std::tuple<A0> &t)
	{
		return f(std::get<0>(t));
	}

	template<typename F,
		typename A0, typename A1>
	auto apply(F &&f, std::tuple<A0,A1> &t)
	{
		return f(std::get<0>(t), std::get<1>(t));
	}

	template<typename F,
		typename A0, typename A1, typename A2>
	auto apply(F &&f, std::tuple<A0,A1,A2> &t)
	{
		return f(std::get<0>(t), std::get<1>(t), std::get<2>(t));
	}
}
