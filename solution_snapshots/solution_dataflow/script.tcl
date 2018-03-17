############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2017 Xilinx, Inc. All Rights Reserved.
############################################################
open_project accelerator
set_top accelerator
add_files ../vgg_16/accelerator/accelerator.cpp -cflags "-I../vgg_16 -DHW_ONLY -std=c++0x"
add_files -tb ../data
add_files -tb ../vgg_16/shared/shared.cpp -cflags "-I../vgg_16 -std=c++0x"
add_files -tb ../vgg_16/accelerator/test/accelerator_test.cpp -cflags "-I../vgg_16 -DHW_ONLY -std=c++0x"
add_files -tb ../vgg_16/accelerator/accelerator.cpp -cflags "-I../vgg_16 -DHW_ONLY -std=c++0x"
open_solution "solution_dataflow"
set_part {xcvu095-ffvc1517-2-e} -tool vivado
create_clock -period 5 -name default
config_compile -name_max_length 30 -pipeline_loops 0 -unsafe_math_optimizations
source "./accelerator/solution_dataflow/directives.tcl"
csim_design -clean -compiler gcc -setup
csynth_design
cosim_design
export_design -rtl verilog -format ip_catalog
