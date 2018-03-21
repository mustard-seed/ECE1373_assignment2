############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2017 Xilinx, Inc. All Rights Reserved.
############################################################
cd hls_proj
open_project acceleratorDummy
set_top accelerator
add_files ../vgg_16/accelerator/accelerator.cpp -cflags "-I../vgg_16 -DHW_ONLY -std=c++0x -DDUMMY"
add_files -tb ../vgg_16/acceleratorDummy/accelerator.cpp -cflags "-I../vgg_16 -DHW_ONLY -std=c++0x -DDUMMY"
add_files -tb ../vgg_16/accelerator/test/accelerator_test.cpp -cflags "-DHW_ONLY -I../vgg_16 -std=c++0x -DDUMMY"
add_files -tb ../vgg_16/accelerator/hw_accelerator.cpp -cflags "-DHW_ONLY -I../vgg_16 -std=c++0x -DDUMMY"
add_files -tb ../vgg_16/shared/shared.cpp -cflags "-I../vgg_16 -std=c++0x -DDUMMY"
add_files -tb ../data

#First solution
open_solution "solution_partitionAndParallel"
set_part {xcvu095-ffvc1517-2-e}
create_clock -period 2.5 -name default
config_compile -name_max_length 30 -pipeline_loops 0 -unsafe_math_optimizations
source "acceleratorPartitionAndParallel_directives.tcl"
#csim_design -argv {0} -compiler gcc -setup
csynth_design
#cosim_design
export_design -rtl verilog -format ip_catalog
close_solution


