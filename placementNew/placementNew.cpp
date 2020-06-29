#include <new>

template<int N = 4>
struct Array
{
	float data[N];	

	Array() = default;

	Array(float f)
	{
		for(auto& a: data)
			a = f;
	}
};

int main()
{
	constexpr int nGangs = 2;
	constexpr int nWorkers = 1;
	#pragma acc parallel num_gangs(nGangs) num_workers(nWorkers)
	{
		#pragma acc loop gang
		for(unsigned int g = 0; g < nGangs; ++g)
		{
			alignas(Array<>) char buf[sizeof(Array<>)];

			#pragma acc loop worker
			for(unsigned int w = 0; w < 1; ++w)
			{
#ifndef ARG_CTOR
				auto array = new (&buf) Array<>;
				array->data[0] = 3.14;
#else
				auto array = new (&buf) Array<>(2.);
				array->data[0] = 3.14;
#endif
			}
		}
	}
}
