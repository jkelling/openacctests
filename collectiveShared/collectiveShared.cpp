#include <iostream>
#include <vector>
#include <climits>

// #define USE_INLINE

__attribute__((noinline))
void sync() {} // dummy sync call

constexpr unsigned int N_GANGS = 2;
#ifdef __PGIC__
#pragma acc routine(sync) bind("__syncthreads")
constexpr unsigned int N_WORKERS = 1024;
#else
constexpr unsigned int N_WORKERS = 1;
#endif

struct Shared
{
	int cache[1024];
};

#pragma acc routine
#ifdef VOIDPTR
inline void funct(size_t nGangs, size_t nWorkers, size_t g, size_t w, size_t i, void* shared, int* io)
#else
inline void funct(size_t nGangs, size_t nWorkers, size_t g, size_t w, size_t i, int* cache, int* io)
#endif
{
#ifdef VOIDPTR
	auto& cache = reinterpret_cast<Shared*>(shared)->cache;
#endif
	/* data-parallel */
	io[g*nWorkers+w] += w+1;

	/* collective load/store */
	// load a pseudo-random number of times to threads out of sync and thusly provoke race conditions
	for(int r = 0; r < ((unsigned int)((w+i*nWorkers)*nGangs+g)*1103515245u)/(double)UINT_MAX*13+1; ++r)
		cache[w] = io[(nGangs+g)*nWorkers+w];
	sync();
	// if(w < 10)
	// 	printf("func: %lld %lld ( %lld ): %lld\n",g,w, nWorkers, (size_t)cache[(w+3)%nWorkers]);
	cache[(w+3)%nWorkers] += (w+3)%nWorkers+1;
	// cache[(w+3)%nWorkers] += w+1;
	// if(w < 10)
	// 	printf("func: %lld %lld : %lld\n",g,w, (size_t)cache[(w+3)%nWorkers]);
	// printf("");
	sync();
	io[(nGangs+g)*nWorkers+w] = cache[w];
}

int main()
{
	constexpr size_t nGangs = N_GANGS;
	constexpr size_t nWorkers = N_WORKERS;

	constexpr size_t N = 2*nGangs*nWorkers;
	int io[N];
	for(int a = 0; a < N; ++a)
		io[a] = 1000000000;

	#pragma acc data copy(io)
	for (int i = 0; i < 1; ++i)
	{
	#pragma acc parallel num_gangs(nGangs) num_workers(nWorkers)
	// #pragma acc parallel num_gangs(nGangs) vector_length(nWorkers)
	{
		#pragma acc loop gang
		for(size_t g = 0; g < nGangs; ++g)
		{
#ifdef VOIDPTR
			Shared shared;
#else
			int cache[nWorkers];
#endif
			// printf("%lld _ ( %lld )\n",g,nWorkers);
			#pragma acc loop worker
			// #pragma acc loop vector
			for(size_t w = 0; w < nWorkers; ++w)
			{
#ifndef USE_INLINE
				// if(w < 10)
				// 	printf("%lld %lld ( %lld )\n",g,w, nWorkers);
#ifdef VOIDPTR
				funct(nGangs, nWorkers, g, w, i, &shared, io);
#else
				funct(nGangs, nWorkers, g, w, i, cache, io);
#endif
#else
				// printf("%lld %lld ( %lld )\n",g,w, (int)nWorkers);
				/* data-parallel */
				io[g*nWorkers+w] += w+1;

				/* collective load/store */
				// load a pseudo-random number of times to threads out of sync and thusly provoke race conditions
				for(int r = 0; r < ((unsigned int)((w+i*nWorkers)*nGangs+g)*1103515245u)/(double)UINT_MAX*13+1; ++r)
					cache[w] = io[(nGangs+g)*nWorkers+w];
				sync();
				cache[(w+3)%nWorkers] += (w+3)%nWorkers+1;
				// cache[(w+3)%nWorkers] += w+1;
				// printf("%lld %lld : %lld\n",g,w, cache[(w+3)%nWorkers]);
				// printf("");
				sync();
				io[(nGangs+g)*nWorkers+w] = cache[w];
#endif
			}
		}
	}
	}

	int a = 0;
	for(int g = 0; g < nGangs; ++g)
	{
		for(int w = 0; w < nWorkers; ++w)
		// for(int w = 0; w < 10; ++w)
		{
			printf("g_%d\tw_%d:\t%d\n", g,w,io[(a*nGangs+g)*nWorkers+w]);
		}
		printf("\n");
	}
	a = 1;
	for(int g = 0; g < nGangs; ++g)
	{
		for(int w = 0; w < nWorkers; ++w)
		// for(int w = 0; w < 10; ++w)
		{
			fprintf(stderr, "g_%d\tw_%d:\t%d\n", g,w,io[(a*nGangs+g)*nWorkers+w]);
		}
		fprintf(stderr, "\n");
	}
}
