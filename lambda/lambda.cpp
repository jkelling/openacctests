#include <iostream>

template<typename F>
void fct(F f)
{
#ifdef DO_CALL
	printf("%d\n", f());
#endif
}

template<typename ...A>
struct NoLambda
{
	int operator() (A ...a) const {return 5;}
};

struct NoLambdaNoT
{
	int operator() () const {return 3;}
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
#if defined(DO_APPLY_FUNCTOR)
				// fct(NoLambda<>());
				fct(NoLambdaNoT());
#else
				fct([](){return 42;});
#endif
			}
		}
	}
}
