#include <cstdio>

#include <iostream>
#include <limits>

template<int TN>
struct MutableN
{
	int operator() () const
	{
		int ret = 0;
		auto N = TN;
		for(int a = 1 ; a < N; ++a)
		{
			ret += 1;
		}
		return ret;
	}
};

template<int TN>
struct ConstN
{
	int operator() () const
	{
		int ret = 0;
		constexpr auto N = TN;
		for(int a = 1 ; a < N; ++a)
		{
			ret += 1;
		}
		return ret;
	}
};

template<class F>
int acc(F f)
{
	constexpr int nGangs = 1;
	constexpr int nWorkers = 1;
	int fret = 0;
	#pragma acc parallel num_gangs(nGangs) num_workers(nWorkers) copy(fret)
	{
		#pragma acc loop gang
		for(unsigned int g = 0; g < nGangs; ++g)
		{
			#pragma acc loop worker
			for(unsigned int w = 0; w < nWorkers; ++w)
			{
				if(g == 0 && w == 0)
					fret = f();
			}
		}
	}
	return fret;
}

template<class F>
bool test(const char* label, F f)
{
	const auto host = f();
	const auto dev = acc(f);
	const bool pass = host == dev;
	std::cout << label << '\t' << host << '\t' << dev << '\t' << (pass ? "pass" : "fail") << '\n';
	return pass;
}

int main()
{
	bool b = true;

	b &= test("ConstN=0", ConstN<0>());
	b &= test("ConstN=1", ConstN<1>());
	b &= test("ConstN=2", ConstN<2>());

	b &= test("MutableN=0", MutableN<0>());
	b &= test("MutableN=1", MutableN<1>());
	b &= test("MutableN=2", MutableN<2>());

	return !b;
}
