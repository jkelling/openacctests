#include <iostream>

#include <openacc.h>


int main()
{
	const auto g = reinterpret_cast<std::uint32_t*>(
		acc_malloc(static_cast<std::size_t>(2 * sizeof(std::uint32_t))));
	std::cout << g << std::endl;

#ifdef ONLY_DEFPRESENT
#    pragma acc parallel loop vector default(present)
	// g not mapped

#elif defined ALL_IMPLICIT
#    pragma acc parallel loop vector
	/* SIGSEGV:
	 * #0  0x00007ffff5572c8a in __c_mcopy4_avx () from /opt/nvidia/hpc_sdk/Linux_x86_64/21.5/compilers/lib/libnvc.so
	 * #1  0x00007ffff7bbcaf0 in __pgi_uacc_move_buffer (dd=0x204d40010) at ../../src/move_buffer.c:80
	 * #2  0x00007ffff7637a9f in __pgi_uacc_cuda_drain_down (devnum=1, qq=32, test=0, tag=0) at ../../src/cuda_drain.c:130
	 * #3  0x00007ffff7642753 in __pgi_uacc_cuda_wait (lineno=-99, async=-1, dindex=1) at ../../src/cuda_wait.c:80
	 * #4  0x00007ffff7b9dfbc in __pgi_uacc_dataexitdone (async=-1, pdevid=0x7fffffffba88, savedevid=1, mode=0) at ../../src/dataexitdone.c:63
	 * #5  0x0000000000401732 in main () at test.cpp:13
	 */

#elif defined COPYIN_DEFPRESENT
#    pragma acc parallel loop vector copyin(g) default(present)
	/* SIGSEGV: (runtime appears to try to copy data behing the pointer, not the pointer itself.)
	 *#0  0x00007ffff7633a7f in __pgi_uacc_cuda_dataup1 (devptr=81672905216,
	 * pbufinfo=0x7fffffffb9b0, hostptr=0x130415a000, offset=0, size=1,
	 * stride=1, elementsize=4, lineno=9, name=0x4018b4 <.S06063> "g",
	 * flags=1792, async=-1, dindex=1) at ../../src/cuda_dataup1.c:102
	 *#1  0x00007ffff7ba4bc4 in __pgi_uacc_dataup1 (devptr=81672905216,
	 * pbufinfo=0x7fffffffb9b0, hostptr=0x130415a000, offset=0, size=1,
	 * stride=1, elementsize=4, lineno=9, name=0x4018b4 <.S06063> "g",
	 * flags=1792, async=-1, devid=1) at ../../src/dataup1.c:50
	 *#2  0x00007ffff7ba526e in __pgi_uacc_dataupx (devptr=81672905216,
	 * pbufinfo=0x7fffffffb9b0, hostptr=0x130415a000, poffset=0, dims=1,
	 * desc=0x7fffffffbac0, elementsize=4, lineno=9, name=0x4018b4 <.S06063>
	 * "g", flags=1792, async=-1, devid=1) at ../../src/dataupx.c:127
	 *#3  0x00007ffff7ba3a05 in __pgi_uacc_dataonb (filename=0x401880
	 * <.F0003.6037> "/tmp/test.cpp", funcname=0x401890 <.F0004.6039> "main",
	 * pdevptr=0x7fffffffba98, hostptr=0x130415a000, hostptrptr=0x7fffffffbaa8,
	 * poffset=0, dims=1, desc=0x7fffffffbac0, elementsize=4, hostdescptr=0x0,
	 * hostdescsize=0, lineno=9, name=0x4018b4 <.S06063> "g", pdtype=0x603fb0
	 * <.y0001>, flags=1792, async=-1, devid=1) at ../../src/dataonb.c:388
	 *#4  0x0000000000401554 in main () at test.cpp:9
	 */
#elif defined CAST
	std::size_t gh = reinterpret_cast<std::size_t>(g);
#    pragma acc parallel loop vector copyin(gh) default(present)
#else
#    error "define one of COPYIN_DEFPRESENT, ALL_IMPLICIT, ONLY_DEFPRESENT, CAST"
#endif
	for(std::size_t a = 0; a < 2; ++a)
	{
#ifdef CAST
		std::uint32_t* g = reinterpret_cast<std::uint32_t*>(gh);
#endif
		g[a] = 0u;
	}
}
