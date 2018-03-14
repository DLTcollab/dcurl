# dcurl - Multi-thread curl implementation
Another implementation for IOTA PearlDiver, which allows to be multi-threaded executed

# Introduction
dcurl use SSE implementation for CPU and OpenCL for GPU, which are referenced from [iotaledger/iota.lib.go](https://github.com/iotaledger/iota.lib.go) and [iotaledger/ccurl](https://github.com/iotaledger/ccurl), respectively.

# Warning
* You need to configure OpenCL platform and device by yourself in ```src/clcontext.c```
* You also need to configure path of OpenCL Library and OpenJDK in ```Makefile```
* dcurl is limited to use only **One** GPU currently, and multi-gpu support is in future work.

# Build
* Build a shared library for IRI
```$ make libdcurl.so```

# Test
* Make a directory for build

```$ mkdir build ```

* Running test program

```$ make test ```

# Tuning
* ```dcurl_init(2, 1)``` in ```jni/iri-pearldiver-exlib.c```
    * ```2``` means 2 pow tasks executed in CPU,
    * ```1``` means 1 pow tasks executed in GPU at the same time.
* Number of threads to find nonce in CPU
    * ```$ export DCURL_NUM_CPU=26```

# Performance 
After integrating dcurl into IRI, performance of <```attachToTangle```> is measured as following.
* Each sampling is measured with 30 transaction trytes and total 200 samples are measured.
* mwm = 14, 26 CPU threads to find nonce
* Settings: enable 2 pow tasks in CPU, 1 pow tasks in GPU at the same time
* [Other Report](https://hackmd.io/s/B1OGHSaDG#)
![](https://lh4.googleusercontent.com/2U_TpfAtEbPdHBcGKD1zl0t0bzo2Rubj0DxXxvV-Rh31Yr7oCCtptutQpLLizMgR7ousEXUtwM6RASnQLOJnGePhQ5Emh1w8l8GlKzMtZ0Yv-TySF2gh3u48BAmllAJv2VjNaxgFGCA)

# About IRI
[Modified IRI accepting external PoW Library](https://github.com/chenwei-tw/iri/tree/feat/new_pow_interface)
* ```$ cd ~/iri && mvn compile clean && mvn package```
* ```$ cp ~/dcurl/libdcurl.so ~/iri```
* ```$ cp ~/dcurl/src/pow_kernel.cl ~/iri/src/```
* ```$ cd ~/iri && java -Djava.library.path=./ -jar target/iri.jar -p <port> --pearldiver-exlib dcurl```

# TODO
* More test program :(
* Pre-compile OpenCL kernel functions and include it in dcurl.
* Automatically configure dcurl after init()

# Externel Source
* ```src/pow_sse.c``` is modifed from [utamaro/curl_trials](https://github.com/utamaro/curl_trials)
* ```src/pow_cl.c``` and ```src/pow_kernel.cl``` is modifed from [iotaledger/ccurl](https://github.com/iotaledger/ccurl)
