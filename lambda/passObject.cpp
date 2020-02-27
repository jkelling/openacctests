#include <iostream>

template<typename F>
#ifdef WORKS
void fct(F &&f)
#else
void fct(F f)
#endif
{
#ifdef DO_CALL
	printf("%d\n", f());
#endif
}

struct NoLambdaNoT
{
#ifdef WORKS_WITH_DATA
	int a = 0;
#endif
#ifdef DO_CALL
	int operator() () const {return 3;}
#endif
};

constexpr size_t nGangs = 2;
constexpr size_t nWorkers = 2;

int main()
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
#if not defined(DO_LAMBDA)
				fct(NoLambdaNoT());
#else
				fct([](){return 42;});
#endif
			}
		}
	}
}
