# FPGA-accelerated backend

## Introduction

dcurl allows FPGA accelerated PoW for radical performance improvements.
With FPGA accelerations, PoW takes 0.14 second in average for MWM=14 and 0.42 second in average for MWM=15.
Both Arrow Sockit board and Intel FPGA DE10-Nano board are known to work with FPGA accelerator.

## Features

The backend were extending fundamental work done by Lampa Lab as following:
* Use Verilog to implement Curl & PoW accelerators;
* Use System Verilog and UVM to verify the accelerators;
* Synthesize Curl & POW hardware accelerators for Intel FPGA DE10-Nano board and validate;
* Write dedicated Linux driver for hardware-accelerated PoW;
More information: [LampaLab/iota_fpga](https://github.com/LampaLab/iota_fpga)

The further work in addition to Lampa Lab done by dcurl developers:
* Resynthesize the POW hardware accelerator for Arrow Sockit board and validate;
* Adapt the PoW hardware accelerator into dcurl backend interface;

## Limitations

* Only one thread is allowed to do FPGA PoW
* No multi-thread management inside the FPGA Implementation Context is implemented
