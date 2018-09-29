# dcurl - Multi-threaded Curl implementation

[![Build Status](https://travis-ci.org/DLTcollab/dcurl.svg?branch=dev)](https://travis-ci.org/DLTcollab/dcurl)
![Supported IRI version](https://img.shields.io/badge/Supported%20IRI%20Version-1.5.3-brightgreen.svg)

Hardware-accelerated implementation for IOTA PearlDiver, which utilizes multi-threaded SIMD, FPGA and GPU.

# Introduction
dcurl exploits SIMD instructions on CPU and OpenCL on GPU. Both CPU and GPU accelerations can be
enabled in multi-threaded execuction fashion, resulting in much faster proof-of-work (PoW) for IOTA
Reference Implementation (IRI). Additionally, dcurl also supports the FPGA-accelerated solution further described in docs/FPGA-ACCEL.md

# Warning
* You need to configure paths and flags of OpenCL installation in ```mk/opencl.mk```
* dcurl will automatically configure all the GPU divices on your platform.
* Check JDK installation and set JAVA_HOME if you wish to specify.
* If your platform doesn't support Intel SSE, dcurl would be compiled with naive implementation.
* For the IOTA hardware accelerator, we integrate [Lampa Lab's Cyclone V FPGA PoW](https://github.com/LampaLab/iota_fpga) into dcurl. Lampa Lab provides soc_system.rbf only for DE10-nano board. You need to synthesize to get soc_system.rbf for using Arrow SoCKit board and [this RBF file](https://github.com/ajblane/dcurl/releases/tag/v1.0-SoCKit) can be downloaded from our release. Moreover, you need to download [Lampa Lab-provided Linux image](https://github.com/LampaLab/iota_fpga/releases/tag/v0.1) to flash into the micro-SD card and root password is 123456. Finally, you also need to download dcurl into root directory.

# Build Instructions
* dcurl allows various combinations of build configurations to fit final use scenarios.
* You can execute `make config` and then edit file `build/local.mk` for custom build options.
    - ``BUILD_AVX``: build Intel AVX-accelerated Curl backend.
    - ``BUILD_GPU``: build OpenCL-based GPU accelerations.
    - ``BUILD_JNI``: build a shared library for IRI. The build system would generate JNI header file
                     downloading from [latest JAVA source](https://github.com/DLTcollab/iri).
    - ``BUILD_COMPAT``: build extra cCurl compatible interface.
    - ``BUILD_FPGA_ACCEL``: build the interface interacting with the Cyclone V FPGA based accelerator. Verified on DE10-nano board and Arrow SoCKit board.
    - ``BUILD_STAT``: show the statistics of the PoW information.
* Alternatively, you can specify conditional build as following:
```shell
$ make BUILD_GPU=0 BUILD_JNI=1 BUILD_AVX=1
```

# Test
* Test with GPU
```shell
$ make BUILD_GPU=1 check
```

* Expected Results
```
*** Validating build/test-trinary ***
        [ Verified ]
*** Validating build/test-curl ***
        [ Verified ]
*** Validating build/test-durl ***
[dcurl] Implementation GPU (OpenCL) is initialized successfully
        [ Verified ]
*** Validating build/test-multi_pow ***
        [ Verified ]
*** Validating build/test-pow ***
GPU - OpenCL
[dcurl] Implementation GPU (OpenCL) is initialized successfully
Success.
        [ Verified ]
```

* Test with AVX but no GPU
```shell
$ make BUILD_AVX=1 check
```

* Expected Results
```
*** Validating build/test-trinary ***
        [ Verified ]
*** Validating build/test-curl ***
        [ Verified ]
*** Validating build/test-dcurl ***
[dcurl] Implementation CPU (Intel AVX) is initialized successfully
        [ Verified ]
*** Validating build/test-multi_pow_cpu ***
        [ Verified ]
*** Validating build/test-pow ***
CPU - AVX
[dcurl] Implementation CPU (Intel AVX) is initialized successfully
Success.
        [ Verified ]
```

* Test with AVX and show the PoW statistics
```shell
$ make BUILD_AVX=1 BUILD_STAT=1 check
```

* Expected Results
```
*** Validating build/test-trinary ***
        [ Verified ]
*** Validating build/test-curl ***
        [ Verified ]
*** Validating build/test-dcurl ***
[dcurl] Implementation CPU (Intel AVX) is initialized successfully
        [ Verified ]
*** Validating build/test-multi-pow ***
[dcurl] Implementation CPU (Intel AVX) is initialized successfully
        [ Verified ]
*** Validating build/test-pow ***
CPU - AVX
[dcurl] Implementation CPU (Intel AVX) is initialized successfully
Hash count: 3182602
PoW execution time: 0.434 sec
Hash rate: 7333.736 kH/sec
Success.
        [ Verified ]
```

* Test with Arrow SoCKit board
```shell
root@lampa:~# sh init_curl_pow.sh 
root@lampa:~# cd dcurl
root@lampa:~/dcurl# make BUILD_STAT=1 BUILD_FPGA_ACCEL=1 check
```

* Expected Results
```
*** Validating build/test-trinary ***
        [ Verified ]
*** Validating build/test-curl ***
        [ Verified ]
*** Validating build/test-dcurl ***
[dcurl] Implementation CPU (Pure C) is initialized successfully
[dcurl] Implementation FPGA is initialized successfully
        [ Verified ]
*** Validating build/test-multi_pow ***
        [ Verified ]
*** Validating build/test-pow ***
CPU - pure C
[dcurl] Implementation CPU (Pure C) is initialized successfully
Hash count: 836032
PoW execution time: 43.000 sec
Hash rate: 19.443 kH/sec
Success.
FPGA
[dcurl] Implementation FPGA is initialized successfully
Hash count: 5125680
PoW execution time: 0.152 sec
Hash rate: 33734.938 kH/sec
Success.
        [ Verified ] 
```

# Tweaks
* Number of threads to find nonce in CPU
    * ```$ export DCURL_NUM_CPU=26```

# Performance
After integrating dcurl into IRI, performance of <```attachToTangle```> is measured as following.
* Each sampling is measured with 30 transaction trytes and total 200 samples are measured.
* mwm = 14, 26 CPU threads to find nonce
* Settings: enable 2 pow tasks in CPU, 1 pow tasks in GPU at the same time
![](https://lh4.googleusercontent.com/2U_TpfAtEbPdHBcGKD1zl0t0bzo2Rubj0DxXxvV-Rh31Yr7oCCtptutQpLLizMgR7ousEXUtwM6RASnQLOJnGePhQ5Emh1w8l8GlKzMtZ0Yv-TySF2gh3u48BAmllAJv2VjNaxgFGCA)

# IRI Adaptation
[Modified IRI accepting external PoW Library](https://github.com/DLTcollab/iri)
Supported IRI version: 1.5.3
* ```$ cd ~/iri && mvn compile && mvn package```
* ```$ cp ~/dcurl/build/libdcurl.so ~/iri```
* ```$ cd ~/iri && java -Djava.library.path=./ -jar target/iri.jar -p <port> --pearldiver-exlib dcurl```

# IOTA PoW Node
[gagathos/iota-gpu-pow](https://github.com/gagathos/iota-gpu-pow)
* You can construct a IOTA PoW node, which uses `ccurl` by default
* Generate a drop-in replacement for `ccurl` and acquire performance boost!
    * ```$ make BUILD_COMPAT=1 check```
    * ```$ cp ./build/libdcurl.so <iota-gpu-pow>/libccurl.so```

# Licensing

`dcurl` is freely redistributable under the MIT License.
Use of this source code is governed by a MIT-style license that can be
found in the `LICENSE` file.
