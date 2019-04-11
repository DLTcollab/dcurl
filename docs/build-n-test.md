# Building and Testing

## Prerequisites
* Check JDK installation and set environment variable `JAVA_HOME` if you wish to specify.
* If target platform lacks of Intel SSE instructions, multi-threaded pure C implementation would be used as the fallback.
* Install the OpenCL and GPU driver before calculating the PoW with GPU.
* For FPGA-based hardware accelerator, [Lampa Lab's Cyclone V FPGA PoW](https://github.com/LampaLab/iota_fpga) is taken as the basis.
     - File `soc_system.rbf` is only valid for DE10-nano board, and you have to synthesize to get appropriate `soc_system.rbf` for Arrow SoCKit board.
     - [RBF file](https://github.com/ajblane/dcurl/releases/tag/v1.0-SoCKit) can be downloaded from our release.
     - Moreover, you need to download [Lampa Lab-provided Linux image](https://github.com/LampaLab/iota_fpga/releases/tag/v0.1) to flash into the micro-SD card. The root password is `123456`.


## Build Instructions
* dcurl allows various combinations of build configurations to fit final use scenarios.
* You can execute `make config` and then edit file `build/local.mk` for custom build options.
    - ``BUILD_AVX``: build the Intel AVX-accelerated Curl backend.
    - ``BUILD_SSE``: build the Intel SSE-accelerated Curl backend.
    - ``BUILD_GPU``: build the OpenCL-based GPU accelerations.
    - ``BUILD_FPGA_ACCEL``: build the interface interacting with the Cyclone V FPGA based accelerator. Verified on DE10-nano board and Arrow SoCKit board.
    - ``BUILD_JNI``: build a shared library for IRI. The build system would generate JNI header file
                     downloading from [latest JAVA source](https://github.com/DLTcollab/iri).
    - ``BUILD_COMPAT``: build extra cCurl compatible interface.
    - ``BUILD_STAT``: show the statistics of the PoW information.
    - ``BUILD_DEBUG``: dump verbose messages internally.
                       Build with the corresponding Sanitizer to detect software potential bugs if the value is `address`, `undefined` or `thread`.
    - ``BOARD``: specify the board which builds the source code.
                 The supported boards are `de10nano` (DE10-nano board), `arrowsockit` (Arrow SoCKit board), `rpi2` and `rpi3` (Raspberry Pi 2/3 board).
* Alternatively, you can specify conditional build as following:
```shell
$ make BUILD_GPU=0 BUILD_JNI=1 BUILD_AVX=1
```

## Fallback Build Instructions
* These instructions do not appear in the file `build/local.mk`.
    - ``BUILD_GENERIC``: build the generic CPU Curl backend without acceleration.

## Testing and Validation
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
        [ Verified ]
*** Validating build/test-multi_pow ***
        [ Verified ]
*** Validating build/test-pow ***
GPU - OpenCL
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
        [ Verified ]
*** Validating build/test-multi_pow_cpu ***
        [ Verified ]
*** Validating build/test-pow ***
CPU - AVX
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
        [ Verified ]
*** Validating build/test-multi-pow ***
        [ Verified ]
*** Validating build/test-pow ***
CPU - AVX
Hash count: 3182602
PoW execution time: 0.434 sec
Hash rate: 7333.736 kH/sec
Success.
        [ Verified ]
```

* Test with Arrow SoCKit board
```shell
$ sh init_curl_pow.sh # set up kernel module
$ make BUILD_STAT=1 BUILD_FPGA_ACCEL=1 check
```

* Expected Results
```
*** Validating build/test-trinary ***
        [ Verified ]
*** Validating build/test-curl ***
        [ Verified ]
*** Validating build/test-dcurl ***
        [ Verified ]
*** Validating build/test-multi_pow ***
        [ Verified ]
*** Validating build/test-pow ***
CPU - pure C
Hash count: 836032
PoW execution time: 43.000 sec
Hash rate: 19.443 kH/sec
Success.
FPGA
Hash count: 5125680
PoW execution time: 0.152 sec
Hash rate: 33734.938 kH/sec
Success.
        [ Verified ] 
```

## Tweaks
* Number of threads to find nonce in CPU
    * ```$ export DCURL_NUM_CPU=26```
