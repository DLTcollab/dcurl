# dcurl - Multi-threaded Curl implementation

[![Build Status](https://travis-ci.org/DLTcollab/dcurl.svg?branch=dev)](https://travis-ci.org/DLTcollab/dcurl)
![Supported IRI version](https://img.shields.io/badge/Supported%20IRI%20Version-1.6.0-brightgreen.svg)

Hardware-accelerated implementation for IOTA PearlDiver, which utilizes multi-threaded SIMD, FPGA and GPU.

## Introduction
dcurl exploits SIMD instructions on CPU and OpenCL on GPU. Both CPU and GPU accelerations can be
enabled in multi-threaded execuction fashion, resulting in much faster [proof-of-work (PoW) for IOTA](https://docs.iota.org/docs/the-tangle/0.1/concepts/proof-of-work).
In addition, dcurl supports FPGA-accelerated PoW, described in [docs/fpga-accelerator.md](docs/fpga-accelerator.md).
dcurl can be regarded as the drop-in replacement for [ccurl](https://github.com/iotaledger/ccurl).
IOTA Reference Implementation (IRI) adaptation is available to benefit from hardware-accelerated PoW.


## Build Instructions
Check [docs/build-n-test.md](docs/build-n-test.md) for details.


## Performance
After integrating dcurl into IRI, performance of <```attachToTangle```> is measured as following.
* Each sampling is measured with 30 transaction trytes and total 200 samples are measured.
* mwm = 14, 26 CPU threads to find nonce
* Settings: enable 2 pow tasks in CPU, 1 pow tasks in GPU at the same time
![](https://lh4.googleusercontent.com/2U_TpfAtEbPdHBcGKD1zl0t0bzo2Rubj0DxXxvV-Rh31Yr7oCCtptutQpLLizMgR7ousEXUtwM6RASnQLOJnGePhQ5Emh1w8l8GlKzMtZ0Yv-TySF2gh3u48BAmllAJv2VjNaxgFGCA)


## IRI Adaptation
[Modified IRI accepting external PoW Library](https://github.com/DLTcollab/iri)
Supported IRI version: 1.6.0
* ```$ cd ~/iri && mvn compile && mvn package```
* ```$ cp ~/dcurl/build/libdcurl.so ~/iri```
* ```$ cd ~/iri && java -Djava.library.path=./ -jar target/iri.jar -p <port> --pearldiver-exlib dcurl```


## IOTA PoW Node
[gagathos/iota-gpu-pow](https://github.com/gagathos/iota-gpu-pow)
* You can construct a IOTA PoW node, which uses `ccurl` by default
* Generate a drop-in replacement for `ccurl` and acquire performance boost!
    * ```$ make BUILD_COMPAT=1 check```
    * ```$ cp ./build/libdcurl.so <iota-gpu-pow>/libccurl.so```


## Licensing
`dcurl` is freely redistributable under the MIT License.
Use of this source code is governed by a MIT-style license that can be
found in the `LICENSE` file.
