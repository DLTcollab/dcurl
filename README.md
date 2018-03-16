# dcurl - Multi-threaded Curl implementation
Hardware-accelerated implementation for IOTA PearlDiver, which utilizes multi-threaded SIMD and GPU.

# Introduction
dcurl exploits SIMD instructions on CPU and OpenCL on GPU. Both CPU and GPU accelerations can be
enabled in multi-threaded execuction fashion, resulting in much faster proof-of-work (PoW) for IOTA
Reference Implementation (IRI).

# Warning
* You need to configure OpenCL platform and device by yourself in ```src/clcontext.c```
* You also need to configure path of OpenCL Library and OpenJDK in ```Makefile```
* Only one GPU can be facilitated with dcurl at the moment.

# Build
* Build a shared library for IRI, generating object files in directory `build`
```shell
$ make
```
Alternatively, you can specify conditional build as following:
```shell
$ make DISABLE_GPU=1 DISABLE_JNI=1
```

# Test
* Make a test

```$ make check ```

* Expected Results

```
*** build/test_trinary ***
	[ Verified ]
*** build/test_curl ***
	[ Verified ]
Testing SSE Implementation with mwm = 14...
Result: KJQEILJFZJYJZBNBTYXNBSNCCMHZDYZXTCHXADBMNPKHFOHNLWJLIGTUHPFEKRZEQ9DZHBJIUJRO99999
*** build/test_pow_sse ***
	[ Verified ]
Testing OpenCL Implementation with mwm = 14...
Result: KJQEILJFZJYJZBNBTYXNBSNCCMHZDYZXTCHXADBMNPKHFOHNLWJLIGTUHPFEKRZEQ9DZHBJIUJRO99999
*** build/test_pow_cl ***
	[ Verified ]
```

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
![](https://lh4.googleusercontent.com/2U_TpfAtEbPdHBcGKD1zl0t0bzo2Rubj0DxXxvV-Rh31Yr7oCCtptutQpLLizMgR7ousEXUtwM6RASnQLOJnGePhQ5Emh1w8l8GlKzMtZ0Yv-TySF2gh3u48BAmllAJv2VjNaxgFGCA)

# About IRI
[Modified IRI accepting external PoW Library](https://github.com/chenwei-tw/iri/tree/feat/new_pow_interface)
* ```$ cd ~/iri && mvn compile clean && mvn package```
* ```$ cp ~/dcurl/libdcurl.so ~/iri```
* ```$ cd ~/iri && java -Djava.library.path=./ -jar target/iri.jar -p <port> --pearldiver-exlib dcurl```

# TODO
* More test program :(
* ~~Pre-compile OpenCL kernel functions and include it in dcurl.~~
* Automatically configure dcurl after init()

# Externel Source
* ```src/pow_sse.c``` is derived from preliminary work of Shinya Yagyu.
* ```src/pow_cl.c``` and ```src/pow_kernel.cl``` are adopted from [iotaledger/ccurl](https://github.com/iotaledger/ccurl).
