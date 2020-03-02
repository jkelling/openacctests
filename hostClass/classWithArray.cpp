#include <iostream>

template<int N>
struct Shared
{
	char c[N];
};

struct Large
{
	double
		a000, a001, a002, a003, a004, a005, a006, a007, a008, a009, a010, a011,
		a012, a013, a014, a015, a016, a017, a018, a019, a020, a021, a022, a023,
		a024, a025, a026, a027, a028, a029, a030, a031, a032, a033, a034, a035,
		a036, a037, a038, a039, a040, a041, a042, a043, a044, a045, a046, a047,
		a048, a049, a050, a051, a052, a053, a054, a055, a056, a057, a058, a059,
		a060, a061, a062, a063, a064, a065, a066, a067, a068, a069, a070, a071,
		a072, a073, a074, a075, a076, a077, a078, a079, a080, a081, a082, a083,
		a084, a085, a086, a087, a088, a089, a090, a091, a092, a093, a094, a095,
		a096, a097, a098, a099, a100, a101, a102, a103, a104, a105, a106, a107,
		a108, a109, a110, a111, a112, a113, a114, a115, a116, a117, a118, a119,
		a120, a121, a122, a123, a124, a125, a126, a127, a128, a129, a130, a131,
		a132, a133, a134, a135, a136, a137, a138, a139, a140, a141, a142, a143,
		a144, a145, a146, a147, a148, a149, a150, a151, a152, a153, a154, a155,
		a156, a157, a158, a159, a160, a161, a162, a163, a164, a165, a166, a167,
		a168, a169, a170, a171, a172, a173, a174, a175, a176, a177, a178, a179,
		a180, a181, a182, a183, a184, a185, a186, a187, a188, a189, a190, a191,
		a192, a193, a194, a195, a196, a197, a198, a199, a200, a201, a202, a203,
		a204, a205, a206, a207, a208, a209, a210, a211, a212, a213, a214, a215,
		a216, a217, a218, a219, a220, a221, a222, a223, a224, a225, a226, a227,
		a228, a229, a230, a231, a232, a233, a234, a235, a236, a237, a238, a239,
		a240, a241, a242, a243, a244, a245, a246, a247, a248, a249, a250, a251,
		a252, a253, a254, a255;
	double
		b000, b001, b002, b003, b004, b005, b006, b007, b008, b009, b010, b011,
		b012, b013, b014, b015, b016, b017, b018, b019, b020, b021, b022, b023,
		b024, b025, b026, b027, b028, b029, b030, b031, b032, b033, b034, b035,
		b036, b037, b038, b039, b040, b041, b042, b043, b044, b045, b046, b047,
		b048, b049, b050, b051, b052, b053, b054, b055, b056, b057, b058, b059,
		b060, b061, b062, b063, b064, b065, b066, b067, b068, b069, b070, b071,
		b072, b073, b074, b075, b076, b077, b078, b079, b080, b081, b082, b083,
		b084, b085, b086, b087, b088, b089, b090, b091, b092, b093, b094, b095,
		b096, b097, b098, b099, b100, b101, b102, b103, b104, b105, b106, b107,
		b108, b109, b110, b111, b112, b113, b114, b115, b116, b117, b118, b119,
		b120, b121, b122, b123, b124, b125, b126, b127, b128, b129, b130, b131,
		b132, b133, b134, b135, b136, b137, b138, b139, b140, b141, b142, b143,
		b144, b145, b146, b147, b148, b149, b150, b151, b152, b153, b154, b155,
		b156, b157, b158, b159, b160, b161, b162, b163, b164, b165, b166, b167,
		b168, b169, b170, b171, b172, b173, b174, b175, b176, b177, b178, b179,
		b180, b181, b182, b183, b184, b185, b186, b187, b188, b189, b190, b191,
		b192, b193, b194, b195, b196, b197, b198, b199, b200, b201, b202, b203,
		b204, b205, b206, b207, b208, b209, b210, b211, b212, b213, b214, b215,
		b216, b217, b218, b219, b220, b221, b222, b223, b224, b225, b226, b227,
		b228, b229, b230, b231, b232, b233, b234, b235, b236, b237, b238, b239,
		b240, b241, b242, b243, b244, b245, b246, b247, b248, b249, b250, b251,
		b252, b253, b254, b255;
};

template<typename T>
struct ClassTemplate
{
	T &t;

	ClassTemplate(T& t)
		: t(t) {}
};

// #define USE_LARGE_CLASS
// #define USE_ARRAY_CLASS
#ifndef SHARED_MEM_SIZE
#define SHARED_MEM_SIZE 2048
#endif

template<class T = int>
void f(size_t nGangs, size_t nWorkers)
{
	#pragma acc parallel num_gangs(nGangs) num_workers(nWorkers)
	// #pragma acc parallel num_gangs(nGangs) vector_length(nWorkers)
	{
		#pragma acc loop gang
		for(int g = 0; g < nGangs; ++g)
		{
#if defined( USE_ARRAY_CLASS ) || defined( USE_LARGE_CLASS )
			T t;
			ClassTemplate<T> ct(t);
#else
			int c[SHARED_MEM_SIZE];
#endif
			// printf("%lld _ ( %lld )\n",g,nWorkers);
			#pragma acc loop worker
			// #pragma acc loop vector
			for(int w = 0; w < nWorkers; ++w)
			{
#ifdef USE_ARRAY_CLASS
				ct.t.c[0] += 23;
#elif defined( USE_LARGE_CLASS )
				ct.t.a000 += 23;
#else
				c[0] += 23;
#endif
			}
		}
	}
}

int main()
{
	constexpr size_t nGangs = 2;
	constexpr size_t nWorkers = 2;

#ifdef USE_ARRAY_CLASS
	f<Shared<SHARED_MEM_SIZE>>(nGangs, nWorkers);
#else
	f<Large>(nGangs, nWorkers);
#endif
}
