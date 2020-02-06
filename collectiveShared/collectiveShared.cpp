#include <cstdio>
#include <vector>
#include <cuda_runtime.h>

#pragma acc routine bind("__syncthreads")
__device__ extern "C" void __syncthreads();

int main()
{
	constexpr size_t nGangs = 1;
	constexpr size_t nWorkers = 64;

	constexpr size_t N = 2*nGangs*nWorkers;
	int io[N];
	for(int a = 0; a < N; ++a)
		io[a] = 1000000000;

	for (int i = 0; i < 1; ++i)
	{
	#pragma acc parallel num_gangs(nGangs) num_workers(nWorkers) copy(io)
	// #pragma acc parallel num_gangs(nGangs) vector_length(nWorkers) copy(io)
	{
		// #pragma acc declare __syncthreads
		#pragma acc loop gang
		for(int g = 0; g < nGangs; ++g)
		{
			int cache[nWorkers];
			// printf("%lld _ ( %lld )\n",g,nWorkers);
			#pragma acc loop worker
			// #pragma acc loop vector
			for(int w = 0; w < nWorkers; ++w)
			{
				// printf("%lld %lld ( %lld )\n",g,w, (int)nWorkers);
				/* data-parallel */
				io[g*nWorkers+w] += w+1;

				/* collective load/store */
				cache[w] = io[(nGangs+g)*nWorkers+w];
				__syncthreads();
				// cache[(w+3)%nWorkers] += (w+3)%nWorkers+1;
				cache[(w+3)%nWorkers] += w+1;
				// printf("%lld %lld : %lld\n",g,w, cache[(w+3)%nWorkers]);
				// printf("");
				__syncthreads();
				io[(nGangs+g)*nWorkers+w] = cache[w];
			}
		}
	}
	}

	for(int a = 0; a < 2; ++a)
	{
		for(int g = 0; g < nGangs; ++g)
		{
			for(int w = 0; w < nWorkers; ++w)
			{
				printf("%d\tg_%d\tw_%d:\t%d\n", a,g,w,io[(a*nGangs+g)*nWorkers+w]);
			}
			printf("\n");
		}
	}
}
