# dcurl - Multi-threaded Curl implementation
Hardware-accelerated implementation for IOTA PearlDiver, which utilizes multi-threaded SIMD and GPU.

# Introduction
dcurl exploits SIMD instructions on CPU and OpenCL on GPU. Both CPU and GPU accelerations can be
enabled in multi-threaded execuction fashion, resulting in much faster proof-of-work (PoW) for IOTA
Reference Implementation (IRI).

# Warning
* You need to configure OpenCL platform and device by yourself in ```src/clcontext.c```
* You need to configure paths and flags of OpenCL installation in ```Makefile```
* Check JDK installation and set JAVA_HOME if you wish to specify.
* Only one GPU can be facilitated with dcurl at the moment.

# Build
* Build a shared library for IRI, generating object files in directory `build`
* Generate JNI header file from downloading from [latest JAVA source](https://github.com/chenwei-tw/iri/tree/feat/new_pow_interface)
```shell
$ make BUILD_JNI=1
```
You can modify `build/local.mk` for custom build options.
Alternatively, you can specify conditional build as following:
```shell
$ make BUILD_GPU=0 BUILD_JNI=1
```

# Test
* Test with GPU
```shell
$ make BUILD_GPU=1 check
```

* Expected Results

```
*** Validating build/test_trinary ***
        [ Verified ]
*** Validating build/test_curl ***
        [ Verified ]
*** Validating build/test_pow_sse ***
        [ Verified ]
*** Validating build/test_pow_cl ***
        [ Verified ]
*** Validating test/test_multi_pow.py ***
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

# Licensing

`dcurl` is freely redistributable under the MIT License.
Use of this source code is governed by a MIT-style license that can be
found in the `LICENSE` file.
