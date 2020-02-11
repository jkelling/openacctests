#include <iostream>

template<int N>
struct HostClass
{
	struct Nested
	{
		int a[N];
	};

	int a;
	template<typename T>
	int operator() (const T &b) const {return a+b;}
	int f(int b) {return a+b;}
};

template<class T, class ...Args>
void f(size_t nGangs, size_t nWorkers, T t, Args ...args)
{
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
				[t](auto ...a)
				{
					printf("%lld\n", t(a...));
				} (args...);
			}
		}
	}
}

int main()
{
	constexpr size_t nGangs = 2;
	constexpr size_t nWorkers = 2;

	HostClass<2> hostClass{5};

	f(nGangs, nWorkers, hostClass, 3);
}
