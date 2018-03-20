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
add_files -tb ../vgg_16/accelerator/test/accelerator_test.cpp -cflags "-DHW_ONLY -I../vgg_16 -std=c++0x"
add_files -tb ../vgg_16/accelerator/accelerator.cpp -cflags "-DHW_ONLY -I../vgg_16 -std=c++0x"
open_solution "solution_partitionAndParallel"
set_part {xcvu095-ffvc1517-2-e}
create_clock -period 5 -name default
config_compile -name_max_length 30 -pipeline_loops 0 -unsafe_math_optimizations
set_directive_array_partition -type complete -dim 1 "accelerator" bufferOutput
set_directive_resource -core RAM_S2P_BRAM "accelerator" bufferOutput
set_directive_array_partition -type complete -dim 1 "accelerator" bufferBroadcast
set_directive_resource -core RAM_S2P_BRAM "accelerator" bufferBroadcast
set_directive_array_partition -type complete -dim 0 "accelerator" bufferCache
set_directive_unroll "util_computeKernel/COMPUTE_FOR_OUTPUT"
set_directive_unroll "util_computeKernel/COMPUTE_FOR_DOT_PRODUCT"
set_directive_array_partition -type complete -dim 0 "convLayer_ComputePartialSum" bufferInputCompute
set_directive_array_partition -type complete -dim 0 "convLayer_ComputePartialSum" bufferOutputCompute
set_directive_pipeline "convLayer_OffloadOutputBuffer/CONVLAYER_OFFLOADRESULT_FOR_N"
set_directive_pipeline "convLayer_LoadBroadcastBuffer/CONVLAYER_LOADBROACASTBUFFER_FOR_LOAD_W"
set_directive_unroll -factor 4 "convLayer_OffloadOutputBuffer/convLayer_OffloadOutputBuffer_label0"
set_directive_array_partition -type complete -dim 0 "convLayer_LoadBroadcastBuffer" bufferDDR
set_directive_dependence -variable bufferBroadcast -type inter -dependent false "convLayer_LoadBroadcastBuffer/CONVLAYER_LOADBROACASTBUFFER_FOR_LOAD_PACKET"
set_directive_dependence -variable bufferBroadcast -type inter -dependent false "convLayer_ComputePartialSum/CONVLAYER_COMPUTE_FOR_INPUTBUFFER_R"
set_directive_unroll "convLayer_ComputePartialSum/CONVLAYER_COMPUTE_FOR_OUTPUTBUFFER_K"
set_directive_unroll "convLayer_ComputePartialSum/CONVLAYER_COMPUTE_FOR_OFFLOAD_K"
set_directive_array_partition -type complete -dim 0 "convLayer_LoadWeights" bufferDDR
set_directive_pipeline "convLayer_LoadBroadcastBuffer/convLayer_LoadBroadcastBuffer_label1"
set_directive_pipeline "convLayer_LoadBroadcastBuffer/convLayer_LoadBroadcastBuffer_label2"
set_directive_pipeline "convLayer_LoadBroadcastBuffer/convLayer_LoadBroadcastBuffer_label3"
set_directive_pipeline "convLayer_LoadBroadcastBuffer/convLayer_LoadBroadcastBuffer_label4"
set_directive_unroll "convLayer_ComputePartialSum/CONVLAYER_COMPUTE_FOR_INPUTBUFFER_C"
set_directive_unroll "convLayer_ComputePartialSum/CONVLAYER_COMPUTE_FOR_INPUTBUFFER_R"
set_directive_unroll "convLayer_ComputePartialSum/CONVLAYER_COMPUTE_FOR_INPUTBUFFER_S"
#csim_design -clean -compiler gcc -setup
csynth_design
#cosim_design
export_design -format ip_catalog
