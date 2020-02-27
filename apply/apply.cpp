#include <iostream>
#include <tuple>

#if __cplusplus < 201703L
#include "meta.h"
namespace std
{
#ifdef USE_MANUAL_APPLY
	#include "apply.h"
#elif defined(USE_DUMMY_APPLY)
	template<typename F>
	auto apply(const F &f, std::tuple<int, int> a)
	{
		printf("USE_DUMMY_APPLY\n");
	}
#elif defined(USE_DUMMY2_APPLY)
	template<typename F,
		typename ...A>
	auto apply(F &&f, std::tuple<A...> a)
	{
	}
#else
	using ::meta::apply;
#endif
}
#endif

template<typename ...A>
int tupleTest(std::tuple<A...> t) {return std::get<0>(t);}

template<typename ...A>
struct NoLambda
{
	void operator() (A ...a) const {}
};

template<typename ...A>
void noLambda(A ...a) {}

constexpr size_t nGangs = 2;
constexpr size_t nWorkers = 2;

template<class ...Args>
void f(Args ...args)
{
	auto targs = std::make_tuple(args...);
	#pragma acc parallel num_gangs(nGangs) num_workers(nWorkers)
	// #pragma acc parallel num_gangs(nGangs) vector_length(nWorkers)
	{
		#pragma acc loop gang
		for(int g = 0; g < nGangs; ++g)
		{
			// printf("%lld _ ( %lld )\n",g,nWorkers);
			#pragma acc loop worker
			// #pragma acc loop vector
			for(int w = 0; w < nWorkers; ++w)
			{
#ifdef DO_APPLY
				std::apply([](auto ...args)
				// std::apply([]()
					{
					}
					, targs);
					// );
#elif defined(DO_APPLY_FUNC_OBJ)
				std::apply(noLambda<Args...>
					, targs);
#elif defined(DO_APPLY_FUNCTOR)
				std::apply(NoLambda<int, int>()
					, targs);
#elif defined(DO_GET)
				printf("%d\n", std::get<0>(targs)+std::get<1>(targs));
				// [](auto ...args){}(std::get<0>(targs),std::get<1>(targs));
#elif defined(DO_TUPLE_TEST)
				printf("%d\n", tupleTest(targs));
#endif
			}
		}
	}
}

int main()
{
	f(2,3);
}
