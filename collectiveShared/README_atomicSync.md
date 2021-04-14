# atomicSync Test

Compile [atomicSync.cpp](atomicSync.cpp) with either OpenMP or OpenACC.

## Flags

### `-DN_GANGS`

Number of gangs to run (just a sequential loop with OpenMP). Default: 1 .

### `-DN_WORKERS`

Number of workers to run per gang (`acc loop worker` or `omp paralel for`). Default: 8 .

### `-DUSE_ATOMIC_SYNC`

Enable atomic-based barrier.

If the barrier works, the output should look like this:
```
g_0     w_0:    1000000001
g_0     w_1:    1000000002
g_0     w_2:    1000000003
g_0     w_3:    1000000004
g_0     w_4:    1000000005
g_0     w_5:    1000000006
g_0     w_6:    1000000007
g_0     w_7:    1000000008

g_0     w_0:    1000000001
g_0     w_1:    1000000002
g_0     w_2:    1000000003
g_0     w_3:    1000000004
g_0     w_4:    1000000005
g_0     w_5:    1000000006
g_0     w_6:    1000000007
g_0     w_7:    1000000008
```
Note, that the second block of numbers (result involving collective operation)
is equal to the first (plain store).

If the barrier is disabled or does not work, the output should look like this:
```
g_0     w_0:    1000000001
g_0     w_1:    1000000002
g_0     w_2:    1000000003
g_0     w_3:    1000000004
g_0     w_4:    1000000005
g_0     w_5:    1000000006
g_0     w_6:    1000000007
g_0     w_7:    1000000008

g_0     w_0:    1000000000
g_0     w_1:    1000000000
g_0     w_2:    1000000000
g_0     w_3:    1000000000
g_0     w_4:    1000000000
g_0     w_5:    1000000000
g_0     w_6:    1000000000
g_0     w_7:    1000000000
```
The second block shows the initial values of the `io` array because threads
do no wait before reading each other's results and may use their own read
values.

Note, that on GPU, the results will look correct within warps, because threads
run in lock-step. Errors will only be visible at warp boundaries (each thread
sends data to a neighbor 3 away).

### `-DUSE_BARRIER`

Will cause compiler error with OpenMP: barrier not allowed in
`omp parallel for`. This is to show, that the implemented synchronization work
in an unsafe setting, under certain conditions.
