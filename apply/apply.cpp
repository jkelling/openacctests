#include <iostream>
#include <tuple>

#if __cplusplus < 201703L
#include "meta.h"
namespace std
{
	using ::meta::apply;
}
#endif

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
				std::apply([](auto ...args)
					{
					}
					, targs);
			}
		}
	}
}

int main()
{
	f(2,3);
}
