# dcurl - Multi-thread curl implementation
Another implementation for IOTA PearlDiver, which allows to be multi-threaded executed

# Introduction
dcurl use SSE implementation for CPU and OpenCL for GPU, which are referenced from [iotaledger/iota.lib.go](https://github.com/iotaledger/iota.lib.go) and [iotaledger/ccurl](https://github.com/iotaledger/ccurl), respectively.

# Warning
* You need to configure OpenCL platform and device by yourself in ```src/clcontext.c```
* You also need to configure OpenCL Library path in ```Makefile```
* dcurl is limited to use only **One** GPU currently, and multi-gpu support is in future work.

# Test
* Make a directory for build

```$ mkdir build ```

* Running test program

```$ make test ```

* Create a multi-thread environment via python

```$ make libdcurl.so```

```$ python3 ./test/test_multi_pow.py 1 1```

# Performance
Runtime is measured with 100 transaction trytes.
![](https://i.imgur.com/iiYkxj2.png)

(8, 7, 32), which means 8 pow tasks executed in CPU, 7 pow tasks executed in GPU **AT THE SAME TIME**, and 32 threads are created to search nonce for each pow task in CPU.

# Externel Source
* ```src/pow_sse.c``` is modifed from [utamaro/curl_trials](https://github.com/utamaro/curl_trials)
* ```src/pow_cl.c``` and ```src/pow_kernel.cl``` is modifed from [iotaledger/ccurl](https://github.com/iotaledger/ccurl)
