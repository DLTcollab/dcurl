# Building and Testing

## Prerequisites
* You need to configure paths and flags of OpenCL installation in `mk/opencl.mk`.
* Check JDK installation and set environment variable `JAVA_HOME` if you wish to specify.
* If target platform lacks of Intel SSE instructions, multi-threaded pure C implementation would be used as the fallback.
* For FPGA-based hardware accelerator, [Lampa Lab's Cyclone V FPGA PoW](https://github.com/LampaLab/iota_fpga) is taken as the basis.
     - File `soc_system.rbf` is only valid for DE10-nano board, and you have to synthesize to get appropriate `soc_system.rbf` for Arrow SoCKit board.
     - [RBF file](https://github.com/ajblane/dcurl/releases/tag/v1.0-SoCKit) can be downloaded from our release.
     - Moreover, you need to download [Lampa Lab-provided Linux image](https://github.com/LampaLab/iota_fpga/releases/tag/v0.1) to flash into the micro-SD card. The root password is `123456`.


## Build Instructions
* dcurl allows various combinations of build configurations to fit final use scenarios.
* You can execute `make config` and then edit file `build/local.mk` for custom build options.
    - ``BUILD_AVX``: build Intel AVX-accelerated Curl backend.
    - ``BUILD_GPU``: build OpenCL-based GPU accelerations.
    - ``BUILD_JNI``: build a shared library for IRI. The build system would generate JNI header file
                     downloading from [latest JAVA source](https://github.com/DLTcollab/iri).
    - ``BUILD_COMPAT``: build extra cCurl compatible interface.
    - ``BUILD_FPGA_ACCEL``: build the interface interacting with the Cyclone V FPGA based accelerator. Verified on DE10-nano board and Arrow SoCKit board.
    - ``BUILD_STAT``: show the statistics of the PoW information.
    - ``BUILD_DEBUG``: dump verbose messages internally.
* Alternatively, you can specify conditional build as following:
```shell
$ make BUILD_GPU=0 BUILD_JNI=1 BUILD_AVX=1
```

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
