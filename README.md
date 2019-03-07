# dcurl - Multi-threaded Curl implementation

[![Build Status](https://travis-ci.org/DLTcollab/dcurl.svg?branch=develop)](https://travis-ci.org/DLTcollab/dcurl)
![Supported IRI version](https://img.shields.io/badge/Supported%20IRI%20Version-1.6.0-brightgreen.svg)
![Release version](https://img.shields.io/github/release-pre/DLTcollab/dcurl.svg)

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
After integrating dcurl into IRI, performance of [attachToTangle](https://iota.readme.io/reference#attachtotangle) is measured as following.
* Each sampling is measured with 30 transaction trytes and total 200 samples are measured.
* mwm = 14, 26 CPU threads to find nonce
* Settings: enable 2 pow tasks in CPU, 1 pow tasks in GPU at the same time
![](https://raw.githubusercontent.com/DLTcollab/dcurl/develop/docs/benchmark.png)


## IRI Adaptation
[Modified IRI accepting external PoW Library](https://github.com/DLTcollab/iri)
Supported IRI version: 1.6.0
* ```$ cd ~/iri && mvn compile && mvn package```
* ```$ cp ~/dcurl/build/libdcurl.so ~/iri```
* ```$ cd ~/iri && java -Djava.library.path=./ -jar target/iri.jar -p <port> --pearldiver-exlib dcurl```


## Adoptions
Here is a partial list of open source projects that have adopted dcurl. If you
are using dcurl and would like your projects added to the list, please send
pull requests to dcurl.

1. [iota-gpu-pow](https://github.com/gagathos/iota-gpu-pow): IOTA PoW node
* You can construct a IOTA PoW node, which uses `ccurl` by default
* Generate a drop-in replacement for `ccurl` and acquire performance boost.
    * ```$ make BUILD_COMPAT=1 check```
    * ```$ cp ./build/libdcurl.so <iota-gpu-pow>/libccurl.so```

2. [iota.keccak.pow.node.js](https://github.com/SteppoFF/iota.keccak.pow.node.js): IOTA PoW node using the iota.keccak.js implementation
* [iota.keccak.js](https://github.com/SteppoFF/iota.keccak.js) can be used to
  build high performance node-js spammers but is also capable of signing
  inputs for value bundles. Using a smart implementation and remote PoW,
  it is capable of performing > 100 TPS for IRI.

3. [IOTA PoW Hardware Accelerator FPGA for Raspberry Pi](https://microengineer.eu/2018/04/25/iota-pearl-diver-fpga/) took dcurl for prototyping.
* Check [shufps/dcurl](https://github.com/shufps/dcurl) and [shufps/iota_vhdl_pow](https://github.com/shufps/iota_vhdl_pow)
  for tracking historical changes outperforming 200x speedup than a Raspberry Pi without hardware hccelerator.

4. [tangle-accelerator](https://github.com/DLTcollab/tangle-accelerator): caching proxy server for IOTA, which can cache API requests and rewrite their responses as needed to be routed through full nodes.
* An instance of Tangle-accelerator can serve thousands of Tangle requests
  at once without accessing remote full nodes frequently.
* As an intermediate server accelerateing interactions with the Tangle,
  it faciliates dcurl to perform hardware-accelerated PoW operations on
  edge devices.

5. [remotepow](https://github.com/tylerw1369/remotepow/tree/Dcurl): delegate PoW to remote servers


## Licensing
`dcurl` is freely redistributable under the MIT License.
Use of this source code is governed by a MIT-style license that can be
found in the `LICENSE` file.
