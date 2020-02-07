#include <iostream>
#include <vector>
#include <climits>

void sync() {} // dummy sync call
#pragma acc routine(sync) bind("__syncthreads")

inline void funct(unsigned int nGangs, unsigned int nWorkers, int g, int w, int i, int* cache, int* io)
{
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
}

int main()
{
	constexpr size_t nGangs = 10;
	constexpr size_t nWorkers = 512;

	constexpr size_t N = 2*nGangs*nWorkers;
	int io[N];
	for(int a = 0; a < N; ++a)
		io[a] = 1000000000;

	#pragma acc data copy(io)
	for (int i = 0; i < 1000; ++i)
	{
	#pragma acc parallel num_gangs(nGangs) num_workers(nWorkers)
	// #pragma acc parallel num_gangs(nGangs) vector_length(nWorkers)
	{
		#pragma acc loop gang
		for(int g = 0; g < nGangs; ++g)
		{
			int cache[nWorkers];
			// printf("%lld _ ( %lld )\n",g,nWorkers);
			#pragma acc loop worker
			// #pragma acc loop vector
			for(int w = 0; w < nWorkers; ++w)
			{
#if 1
				funct(nGangs, nWorkers, g, w, i, cache, io);
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
		{
			printf("g_%d\tw_%d:\t%d\n", g,w,io[(a*nGangs+g)*nWorkers+w]);
		}
		printf("\n");
	}
	a = 1;
	for(int g = 0; g < nGangs; ++g)
	{
		for(int w = 0; w < nWorkers; ++w)
		{
			fprintf(stderr, "g_%d\tw_%d:\t%d\n", g,w,io[(a*nGangs+g)*nWorkers+w]);
		}
		fprintf(stderr, "\n");
	}
}
