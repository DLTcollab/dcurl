
# IOTA FPGA-accelerated solution for Dcurl

This document is divided into tow parts. The first part is to describe motivation and contribution and the other part is to roughly state IOTA FPGA-accelerated integration into dcurl.

dcurl supports IOTA FPGA-accelerated solutions to improve PoW performance. PoW calculation time for MWM=14 is between 0.001 and 0.8 second and 0.14 second in average and The time for MWM=15 is between 0.01 and 2 second and 0.42 second in average. Currently, it is experimented and verified on Arrow Sockit board and Intel FPGA DE10-Nano board. We reuse the Lampa Lab-provided FPGA-accelerated solution. 

Here is a brief summary of the tasks Lampa Lab have done:
* Use Verilog to implement Curl & POW accelerators
* Use System Verilog and UVM to verify the accelerators
* Synthesize Curl & POW hardware accelerators for Intel FPGA DE10-Nano board and flash it into the board
* Write Linux drivers in Gloden System Reference Design for Curl & POW hardware accelerators and verify them
More information: [LampaLab/iota_fpga](https://github.com/LampaLab/iota_fpga)

Here is a brief summary of the tasks we have done:
* Resynthesize the POW hardware accelerator for Arrow Sockit board and flash it into the board
* Integrate the IOTA PoW hardware accelerator into dcurl's implementation interface
* Test and verify it

For IOTA FPGA-accelerated integration into dcurl, we consider one dcurl maps one FPGA board unlike the GPU-accelerated solution which is considered a multi-GPU scenario. 

Here is a simple summary of integrating FPGA-accelerated solution into dcurl:
* Only one thread is allowed to do FPGA PoW
* No multi-thread management inside the FPGA Implementation Context is implemented
