#include <iostream>
#include <vector>
#include <climits>
#include <cassert>
#include <memory>

#define USE_TUPLE
#ifdef USE_TUPLE
#include <tuple>
#endif

#ifdef USE_MANUAL_APPLY
	#include "apply.h"
#else
#if __cplusplus < 201703L
#include <meta.h>
namespace std
{
	using ::meta::apply;
}
#endif
#endif

template<typename Shared>
struct AccGeneric
{
	unsigned int nGangs, nWorkers;
	unsigned int g, w;

	Shared *shared;

		AccGeneric(unsigned int nGangs, unsigned int nWorkers
				, unsigned int g, unsigned int w, Shared *shared)
		: nGangs(nGangs), nWorkers(nWorkers), g(g), w(w), shared(shared)
	{}

	void __syncthreads() const {}
};

#ifdef _OPENACC

template<typename F, typename ...Args>
void invokeKernel(F&& f, Args &&...args)
{
	f(std::forward(args)...);
}

#include <openacc.h>

constexpr unsigned int N_GANGS = 2;
__attribute__((noinline))
void syncOpenACC() {} // dummy sync call

#ifdef __PGIC__
#pragma acc routine(syncOpenACC) bind("__syncthreads")
constexpr unsigned int N_WORKERS = 128;
#else
constexpr unsigned int N_WORKERS = 1;
#endif

template<typename Shared>
struct AccOpenACC : public AccGeneric<Shared>
{
	using AccGeneric<Shared>::AccGeneric;
	void __syncthreads() const {syncOpenACC();}
};

struct AccOpenACCNoT
{
	unsigned int nGangs, nWorkers;
	unsigned int g, w;

	void* shared;

		AccOpenACCNoT(unsigned int nGangs, unsigned int nWorkers
				, unsigned int g, unsigned int w, void* shared)
		: nGangs(nGangs), nWorkers(nWorkers), g(g), w(w)
	{}
	void __syncthreads() const {syncOpenACC();}
};

template<typename Acc, typename Funct, typename ...Args>
void execKernelOpenACCInner(const Funct &pfunct, unsigned int pnGangs, unsigned int pnWorkers, Args ...args)
{
#ifdef USE_TUPLE
	const std::tuple<std::decay_t<Args>...> targs(std::forward<Args>(args)...);
#endif
	const auto funct = pfunct;
	const auto nGangs = pnGangs;
	const auto nWorkers = pnWorkers;
	#pragma acc parallel num_gangs(nGangs) num_workers(nWorkers)
	{
		#pragma acc loop gang
		for(unsigned int g = 0; g < nGangs; ++g)
		{
			typename std::decay<Funct>::type::Shared shared;
			// AccOpenACC<typename Funct::Shared> acc(nGangs, nWorkers, g, 0);
			Acc acc(nGangs, nWorkers, g, 0, &shared);
			// printf("%lld _ ( %lld )\n",g,nWorkers);
			#pragma acc loop worker
			for(int w = 0; w < nWorkers; ++w)
			{
				auto wacc = acc;
				wacc.w = w;
#ifdef USE_TUPLE
				std::apply([&wacc, funct](typename std::decay<Args>::type const & ... args)
					{
						funct(wacc, args...);
					}, targs);
#else
				funct(wacc, std::forward<Args>(args)...);
#endif
			}
		}
	}
}

template<typename Funct, typename ...Args>
void execKernelOpenACC(const Funct &funct, unsigned int nGangs, unsigned int nWorkers, Args ...args)
{
	execKernelOpenACCInner<AccOpenACC<typename Funct::Shared>>(funct, nGangs, nWorkers, std::forward<Args>(args)...);
}

// using OpenACC "backend"
template<typename ...Args>
void exec(Args ...args)
{
	execKernelOpenACC(std::forward<Args>(args)...);
}

// #define DEV_FUNCT _Pragma ("acc routine") \ // does not work
// 	inline
#define DEV_FUNCT inline

#elif defined _OPENMP

#include <omp.h>

constexpr unsigned int N_GANGS = 2;
constexpr unsigned int N_WORKERS = 128;

template<typename Shared>
struct AccOpenMP : public AccGeneric<Shared>
{
	using AccGeneric<Shared>::AccGeneric;
	void __syncthreads() const {
#pragma omp barrier
	}
};

template<typename Acc, typename Funct, typename ...Args>
void execKernelOpenMPInner(const Funct &pfunct, unsigned int pnGangs, unsigned int pnWorkers, Args ...args)
{
#ifdef USE_TUPLE
	const std::tuple<std::decay_t<Args>...> targs(std::forward<Args>(args)...);
#endif
	const auto funct = pfunct;
	const auto nGangs = pnGangs;
	const auto nWorkers = pnWorkers;
	#pragma omp target
	{
		#pragma omp teams num_teams(nGangs) thread_limit(nWorkers)
		{
			#pragma omp distribute
			for(unsigned int g = 0; g < nGangs; ++g)
			{
				// auto shared = std::make_unique<typename std::decay<Funct>::type::Shared>();
				// Acc acc(nGangs, nWorkers, g, 0, &*shared);
				typename std::decay<Funct>::type::Shared shared;
				Acc acc(nGangs, nWorkers, g, 0, &shared);
				// printf("%lld _ ( %lld )\n",g,nWorkers);
				#pragma omp parallel num_threads(nWorkers)
				{
					auto wacc = acc;
					wacc.w = omp_get_thread_num();
					// printf("OMP num threads=%d:%d, %d:%d\n", ::omp_get_num_threads(), nWorkers, omp_get_thread_num(), wacc.w);
#if 1
#ifdef USE_TUPLE
					std::apply([&wacc, funct](typename std::decay<Args>::type const & ... args)
						{
							funct(wacc, args...);
						}, targs);
#else
					funct(wacc, std::forward<Args>(args)...);
#endif
#endif
				}
			}
		}
	}
}

template<typename Funct, typename ...Args>
void execKernelOpenMP(const Funct &funct, unsigned int nGangs, unsigned int nWorkers, Args ...args)
{
	execKernelOpenMPInner<AccOpenMP<typename Funct::Shared>>(funct, nGangs, nWorkers, std::forward<Args>(args)...);
}

// using OpenACC "backend"
template<typename ...Args>
void exec(Args ...args)
{
	execKernelOpenMP(std::forward<Args>(args)...);
}

#define DEV_FUNCT inline

#else

constexpr unsigned int N_GANGS = 2;
constexpr unsigned int N_WORKERS = 1;

template<typename Funct, typename ...Args>
void execKernelCPUseq(Funct funct, unsigned int nGangs, unsigned int nWorkers, Args ...args)
{
	for(unsigned int g = 0; g < nGangs; ++g)
	{
		typename Funct::Shared shared;
		AccGeneric<typename Funct::Shared> acc = {nGangs, nWorkers, g, 0, shared};
		funct(acc, std::forward<Args>(args)...);
	}
}

// using CPU "backend"
template<typename ...Args>
void exec(Args ...args)
{
	execKernelCPUseq(std::forward<Args>(args)...);
}

#define DEV_FUNCT inline

#endif

template<int MaxWorkers = 256, typename T = int>
struct Funct
{
	int a = 0;
	struct Shared
	{
		int cache[MaxWorkers];

			Shared() = default;
	};

#if 1
	template<typename Acc>
	DEV_FUNCT
	void operator() (const Acc& acc, int* io, int i) const
	{
		const auto& nGangs = acc.nGangs;
		const auto& nWorkers = acc.nWorkers;
		const auto& g = acc.g;
		const auto& w = acc.w;
		auto& cache = acc.shared->cache;

		// printf("%lld %lld ( %lld )\n",g,w, (int)nWorkers);
		/* data-parallel */
		io[g*nWorkers+w] += 10000 + w;

		/* collective load/store */
		// load a pseudo-random number of times to threads out of sync and thusly provoke race conditions
		for(int r = 0; r < ((unsigned int)((w+i*nWorkers)*nGangs+g)*1103515245u)/(double)UINT_MAX*13+1; ++r)
			cache[w] = io[(nGangs+g)*nWorkers+w];
		acc.__syncthreads();
		constexpr int shift = 3;
		cache[(w+shift)%nWorkers] += 10000+(w+shift)%nWorkers;
		acc.__syncthreads();
		io[(nGangs+g)*nWorkers+w] = cache[w];
	}
#endif
};


int main()
{
	constexpr auto nGangs = N_GANGS;
	constexpr auto nWorkers = N_WORKERS;

	constexpr size_t N = 2*nGangs*nWorkers;
	int io[N];
	for(int a = 0; a < N; ++a)
		io[a] = 1000000000;

#ifdef _OPENACC // leaving out abstract memory management here
	#pragma acc data copy(io)
	for (int i = 0; i < 1; ++i)
	{
		exec(Funct<>(), nGangs, nWorkers, (int*)acc_deviceptr(io), i);
	}
#elif defined _OPENMP
	int* dio = (int*)omp_target_alloc(sizeof(io), 0);
	omp_target_memcpy(dio, io, sizeof(io), 0,0,0, omp_get_initial_device());
	for (int i = 0; i < 1; ++i)
	{
		exec(Funct<>(), nGangs, nWorkers, dio, i);
	}
	omp_target_memcpy(io, dio, sizeof(io), 0,0,omp_get_initial_device(),0);
#else
	for (int i = 0; i < 1; ++i)
	{
		exec(Funct<>(), nGangs, nWorkers, io, i);
	}
#endif

	int a = 0;
	printf("BEGIN\n");
	for(int g = 0; g < nGangs; ++g)
	{
		for(int w = 0; w < nWorkers; ++w)
		{
			printf("g_%d\tw_%d:\t%d\n", g,w,io[(a*nGangs+g)*nWorkers+w]);
		}
		printf("\n");
	}
	a = 1;
	fprintf(stderr, "BEGIN\n");
	for(int g = 0; g < nGangs; ++g)
	{
		for(int w = 0; w < nWorkers; ++w)
		{
			fprintf(stderr, "g_%d\tw_%d:\t%d\n", g,w,io[(a*nGangs+g)*nWorkers+w]);
		}
		fprintf(stderr, "\n");
	}
}
