# dcurl - Multi-thread curl implementation
Another implementation for IOTA PearlDiver, which allows to be multi-threaded executed

<br>

# Introduction
dcurl use SSE implementation for CPU and OpenCL for GPU, which are referenced from [iotaledger/iota.lib.go](https://github.com/iotaledger/iota.lib.go) and [iotaledger/ccurl](https://github.com/iotaledger/ccurl), respectively.

<br>

# Warning
* You need to configure OpenCL platform and device by yourself in ```src/clcontext.c```
* You also need to configure OpenCL Library path iin ```Makefile```
* dcurl is limited to use only **One** GPU currently, and multi-gpu support is in future work.

<br>

# Test
* Make a directory for build
```$ mkdir build ```
* Try a IOTA transaction tryte
```$ make test && ./test ```
* Create a multi-thread environment via python
```$ make libdcurl.so```
```$ python3 ./test/test__multi_pow.py```

<br>

# Performance
Runtime is measured with 100 transaction trytes.
![](https://i.imgur.com/iiYkxj2.png)

(8, 7, 32), which means 8 pow tasks executed in CPU, 7 pow tasks executed in GPU AT THE SAME TIME, and 32 threads are created to search nonce for each pow task.
