# ECE1373_assignment2

Welcome to the submission for the second assignment in ECE1373

This will describe how to run the provided sample code in this directory. 

## Code Organization
-IMPORTANT!!!!! THE SOURCE CODES THAT CAME WITH THE ASSIGNMENT START UP FILES WERE NOT USED!
-IMPORTANT!!!!! Instead of 8v3_shell/static_routed_v1.dcp, this submission uses 8v3_shell/static_routed_v2.dcp

The source code is organized as follows:
- vgg_16/accelerator directory has files for the implemented accelerator
- vgg_16/common has files that define paramters and data types
- vgg)16/shared contains files used during tests
- nn_params stores binaries for the weights, biases, inputs and reference output. This also contains a script extractParams to create new binaries for other layers. 
- hls_proj contains tcl scripts to create a vivado_hls project for accelerator.
- pci_tests includes tests to read and write to pcie.
- 8v3_shell contains files and projects for the hypervisor and user appplications, as well as the bitstreams.

## Where to find the bitstreams for the 3 implementations
The 3 implementations are 
1) Array Partition and Parallel
- Implementation bit stream: 8v3_shell/accelerator_pr_pblock_pr_region_partial.bit
- Clear bit stream: 8v3_shell/accelerator_pr_pblock_pr_region_partial_clear.bit

2) Parallel without Array Partition
- Implementation bit stream: 8v3_shell/acceleratorNoPartition_pr_pblock_pr_region_partial.bit
- Clear bit stream: 8v3_shell/acceleratorNoPartition_pr_pblock_pr_region_partial_clear.bit

3) Ping-pong
- Implementation bit stream: 8v3_shell/acceleratorFlow_pr_pblock_pr_region_partial.bit
- Clear bit stream: 8v3_shell/acceleratorFlow_pr_pblock_pr_region_partial_clear.bit

4) Dummy (It is only used to get latencies due to memory access. It is not really an implementation, as it does not actually perform convolution)
- Implementation bit stream: 8v3_shell/acceleratorDummy_pr_pblock_pr_region_partial.bit
- Clear bit stream: 8v3_shell/acceleratorDummy_pr_pblock_pr_region_partial_clear.bit


## How to make and run tests
A test executable has been generated that runs 1 batch of 10 images through all every convolutional layers in the VGG-16 network
- How o make the exeutable: In the project folder's root, type 
make hw_accelerator
An executable named hw_accelerator should be created in the project root directory

- How to run tests on CPU
./hw_accelerator [any character]

- How to run tests on FPGA (Important: make sure you've programmed the FPGA first!)
./hw_accelerator 7


## How to re-generate the 3 implmentations' bit streams (Do this only when absolutely necessary and abudant time is available)
make accelerator_createHW

## How to re-generate the dummy implementation's bit stream
make acceleratorDummy_createHW

