#include <iostream>
#include <vector>
#include <climits>
#include <cassert>

#include <tuple>

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

template<typename Shared>
struct AccOpenACC : public AccGeneric<Shared>
{
	using AccGeneric<Shared>::AccGeneric;
	// void __syncthreads() const {syncOpenACC();}
};

template<typename Acc, typename Funct, typename ...Args>
void execKernelOpenACCInner(Funct &&funct, unsigned int nGangs, unsigned int nWorkers, const std::tuple<Args...>& targs);
// template<typename Acc, typename Funct, typename TArgs>
// void execKernelOpenACCInner(Acc &&acc, Funct &&funct, unsigned int nGangs, unsigned int nWorkers, const TArgs& targs);

template<typename Funct, typename ...Args>
void execKernelOpenACC(Funct &&funct, unsigned int nGangs, unsigned int nWorkers, Args ...args)
{
	auto targs = std::make_tuple(args...);
	execKernelOpenACCInner<AccOpenACC<typename Funct::Shared>>(funct, nGangs, nWorkers, targs);
	// typename Funct::Shared shared;
	// execKernelOpenACCInner(AccOpenACC<typename Funct::Shared>(nGangs, nWorkers, 0u, 0u, shared), funct, nGangs, nWorkers, targs);
}

template<typename Acc, typename Funct, typename ...Args>
void execKernelOpenACCInner(Funct &&funct, unsigned int nGangs, unsigned int nWorkers, const std::tuple<Args...>& targs)
// template<typename Acc, class Funct, typename TArgs>
// void execKernelOpenACCInner(Acc &&acc, Funct &&funct, unsigned int nGangs, unsigned int nWorkers, const TArgs& targs)
// template<class Funct, typename TArgs>
// void execKernelOpenACCInner(Funct &&funct, unsigned int nGangs, unsigned int nWorkers, const TArgs& targs)
{
	typename std::decay<Funct>::type::Shared shared;
}

// using OpenACC "backend"
template<typename ...Args>
void exec(Args ...args)
{
	execKernelOpenACC(std::forward<Args>(args)...);
}

constexpr unsigned int N_GANGS = 2;
constexpr unsigned int N_WORKERS = 32;

// #define DEV_FUNCT _Pragma ("acc routine") \ // does not work
// 	inline
#define DEV_FUNCT inline

template<int MaxWorkers = 512, typename T = int>
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
		auto& cache = reinterpret_cast<Shared*>(acc.shared)->cache;

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

	for (int i = 0; i < 1; ++i)
	{
		exec(Funct<>(), nGangs, nWorkers, io, i);
	}
}
