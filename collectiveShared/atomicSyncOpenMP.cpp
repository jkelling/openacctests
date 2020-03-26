#include <iostream>
#include <vector>
#include <climits>

#include <omp.h>

constexpr unsigned int N_GANGS = 1;
struct Sync
{
	int m_generation = 0;
	short m_syncCounter[4] {0,0,0,0};

	void sync(int workerNum)
	{
		const auto slot = (m_generation&1)<<1;
		int sum;
		#pragma omp atomic capture
		sum = ++m_syncCounter[slot];
		// printf("thread %d reached barrier sum=%d\n", omp_get_thread_num(), sum);
		if(sum == workerNum)
		{
			++m_generation;
			const int nextSlot = (m_generation&1)<<1;
			m_syncCounter[nextSlot] = 0;
			m_syncCounter[nextSlot+1] = 0;
			// op();
		}
		while(sum < workerNum)
		{
			#pragma omp atomic read
			sum = m_syncCounter[slot];
			// printf("thread %d waiting sum=%d\n", omp_get_thread_num(), sum);
		}
		// printf("thread %d woke up at barrier sum=%d\n", omp_get_thread_num(), sum);
		#pragma omp atomic capture
		sum = ++m_syncCounter[slot+1];
		while(sum < workerNum)
		{
			#pragma omp atomic read
			sum = m_syncCounter[slot+1];
		}
		// printf("thread %d departed barrier sum=%d nextgen=%d\n", omp_get_thread_num(), sum, m_generation);
	}
};
constexpr unsigned int N_WORKERS = 8;

int main()
{
	constexpr size_t nGangs = N_GANGS;
	constexpr size_t nWorkers = N_WORKERS;

	constexpr size_t N = 2*nGangs*nWorkers;
	int io[N];
	for(int a = 0; a < N; ++a)
		io[a] = 1000000000;

	int i = 1;
	int cache[nWorkers];
#ifdef USE_ATOMIC_SYNC
	Sync sync;
#endif
	// printf("%lld _ ( %lld )\n",g,nWorkers);
	int g = 0;
	#pragma omp parallel for
	for(size_t w = 0; w < nWorkers; ++w)
	{
		printf("%d %d %d\n", g, w, omp_get_thread_num());
		/* data-parallel */
		io[g*nWorkers+w] += w+1;

		/* collective load/store */
		// load a pseudo-random number of times to threads out of sync and thusly provoke race conditions
		for(int r = 0; r < ((unsigned int)((w+i*nWorkers)*nGangs+g)*1103515245u)/(double)UINT_MAX*13+1; ++r)
			cache[w] = io[(nGangs+g)*nWorkers+w];
#ifdef USE_ATOMIC_SYNC
		sync.sync(nWorkers);
#else
#pragma omp barrier
#endif
		cache[(w+3)%nWorkers] += (w+3)%nWorkers+1;
		// cache[(w+3)%nWorkers] += w+1;
		// printf("%lld %lld : %lld\n",g,w, cache[(w+3)%nWorkers]);
		// printf("");
#ifdef USE_ATOMIC_SYNC
		sync.sync(nWorkers);
#else
#pragma omp barrier
#endif
		io[(nGangs+g)*nWorkers+w] = cache[w];
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
