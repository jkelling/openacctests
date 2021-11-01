#include <iostream>

static constexpr int NGANGS = 8;
static constexpr int NWORKERS = 64;
static constexpr int ARRSIZE = 40<<10;

int main()
{

	int data[NGANGS];

	for(auto& a : data)
		a = 0;

#	pragma acc parallel copy(data[0:NGANGS])
	{
#		pragma acc loop gang 
		for(int g = 0; g < NGANGS; ++g)
		{
			int gangLocal[ARRSIZE];
#			pragma acc loop worker
			for(auto& a : gangLocal)
				a = 0;
#			pragma acc loop worker
			for(int w = 0; w < NWORKERS; ++w)
			{
#				pragma acc atomic update
				gangLocal[ARRSIZE/2] += 1;
			}
			data[g] += gangLocal[ARRSIZE/2];
		}
	}

	bool fail = false;
	// fail with NVHPC 21.9 -acc -ta=tesla -O >= 2
	for(int g = 0; g < NGANGS; ++g)
	{
		if(data[g] != NWORKERS)
		{
			fail = true;
			std::cout << "data[ " << g << " ] contains wrong sum: " << data[g] << '\n';
		}
	}

	return fail;
}
