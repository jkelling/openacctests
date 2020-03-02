#include <iostream>
#include <vector>
#include <climits>
#include <cassert>

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

	Shared &shared;

		AccGeneric(unsigned int nGangs, unsigned int nWorkers
				, unsigned int g, unsigned int w, Shared shared)
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

// template<typename Acc, typename Funct, typename ...Args>
// void execKernelOpenACCInner(Funct &&funct, unsigned int nGangs, unsigned int nWorkers, const std::tuple<Args...>& targs);
template<typename Acc, typename Funct, typename Tuple>
void execKernelOpenACCInner(Funct &&funct, unsigned int nGangs, unsigned int nWorkers, const Tuple& targs);

template<typename Funct, typename ...Args>
void execKernelOpenACC(Funct &&funct, unsigned int nGangs, unsigned int nWorkers, Args ...args)
{
#ifdef USE_TUPLE
#if defined(DO_APPLY_FUNC_OBJ)
	auto targs = std::make_tuple(args...);
#elif defined(INCLUDE_FUNCT_IN_TUPLE)
	auto targs = std::make_tuple(funct, args...);
#else
	auto targs = std::make_tuple(args...);
#endif
#endif
	execKernelOpenACCInner<AccOpenACC<typename Funct::Shared>>(funct, nGangs, nWorkers, targs);
}

template<typename Acc, typename Funct, typename Tuple>
void execKernelOpenACCInner(Funct &&funct, unsigned int pnGangs, unsigned int pnWorkers, const Tuple& targs)
{
	const auto dtargs = targs;
	const auto nGangs = pnGangs;
	const auto nWorkers = pnWorkers;
	#pragma acc parallel num_gangs(nGangs) num_workers(nWorkers)
	{
		#pragma acc loop gang
		for(unsigned int g = 0; g < nGangs; ++g)
		{
			typename std::decay<Funct>::type::Shared shared;
			// AccOpenACC<typename Funct::Shared> acc(nGangs, nWorkers, g, 0);
			Acc acc(nGangs, nWorkers, g, 0, shared);
			// printf("%lld _ ( %lld )\n",g,nWorkers);
			#pragma acc loop worker
			for(int w = 0; w < nWorkers; ++w)
			{
				auto wacc = acc;
				wacc.w = w;
#ifdef USE_TUPLE
#if defined(DO_APPLY_FUNC_OBJ)
				auto tup = std::make_tuple(funct, wacc);
				// auto tup = std::tuple_cat(std::make_tuple(funct, wacc), targs);
				// std::apply(invokeKernel<Funct, AccOpenACC<typename Funct::Shared>, Args...>
				// 	, std::tuple_cat(std::make_tuple(funct, wacc), targs));
#else
				std::apply([&wacc, funct](auto &&...args)
				// std::apply([](auto &&funct, auto &&...args)
				// std::apply([&wacc, funct](int* io, int i)
					{
						funct(wacc, args...);
						// funct(wacc, io, i);
					}, dtargs);
#endif
#else
				funct(wacc, std::forward<Args>(args)...);
#endif
			}
		}
	}
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
	void operator() (Acc& acc, int* io, int i) const
	{
		auto& nGangs = acc.nGangs;
		auto& nWorkers = acc.nWorkers;
		assert(nWorkers <= MaxWorkers);
		auto& g = acc.g;
		auto& w = acc.w;
		auto& cache = acc.shared.cache;

		// printf("%lld %lld ( %lld )\n",g,w, (int)nWorkers);
		/* data-parallel */
		io[g*nWorkers+w] += w+1;

		/* collective load/store */
		// load a pseudo-random number of times to threads out of sync and thusly provoke race conditions
		for(int r = 0; r < ((unsigned int)((w+i*nWorkers)*nGangs+g)*1103515245u)/(double)UINT_MAX*13+1; ++r)
			cache[w] = io[(nGangs+g)*nWorkers+w];
		acc.__syncthreads();
		cache[(w+3)%nWorkers] += (w+3)%nWorkers+1;
		// cache[(w+3)%nWorkers] += w+1;
		// printf("%lld %lld : %lld\n",g,w, cache[(w+3)%nWorkers]);
		// printf("");
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
#else
	for (int i = 0; i < 1000; ++i)
	{
		exec(Funct<>(), nGangs, nWorkers, io, i);
	}
#endif

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
