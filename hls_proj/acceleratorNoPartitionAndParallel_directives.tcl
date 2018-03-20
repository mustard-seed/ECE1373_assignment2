############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2017 Xilinx, Inc. All Rights Reserved.
############################################################
set_directive_resource -core RAM_S2P_BRAM "accelerator" bufferOutput
set_directive_resource -core RAM_S2P_BRAM "accelerator" bufferBroadcast
set_directive_array_partition -type complete -dim 0 "accelerator" bufferCache
set_directive_unroll "util_computeKernel/COMPUTE_FOR_OUTPUT"
set_directive_unroll "util_computeKernel/COMPUTE_FOR_DOT_PRODUCT"
set_directive_array_partition -type complete -dim 0 "convLayer_ComputePartialSum" bufferInputCompute
set_directive_array_partition -type complete -dim 0 "convLayer_ComputePartialSum" bufferOutputCompute
set_directive_pipeline "convLayer_LoadBroadcastBuffer/CONVLAYER_LOADBROACASTBUFFER_FOR_LOAD_W"
set_directive_pipeline "convLayer_OffloadOutputBuffer/convLayer_OffloadOutputBuffer_label0"
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
set_directive_pipeline "convLayer_LoadBroadcastBuffer/convLayer_LoadBroadcastBuffer_label0"
set_directive_pipeline "convLayer_PrepareOutputBuffer/CONVLAYER_PREPAREOUTPUT_FOR_N"
set_directive_pipeline "convLayer_OffloadOutputBuffer/CONVLAYER_OFFLOADRESULT_FOR_PACKET"
set_directive_pipeline "convLayer_ComputePartialSum/CONVLAYER_COMPUTE_FOR_N"
set_directive_inline "convLayer_ComputePartialSum"
set_directive_inline "convLayer_WrapperLoadWeightsAndInputs"
set_directive_pipeline "convLayer_LoadWeights/CONVLAYER_LOADWEIGHTS_FOR_PARALLEL_K"
set_directive_pipeline "convLayer_OffloadOutputBuffer/CONVLAYER_OFFLOADRESULT_FOR_N"
set_directive_inline -off "util_computeKernel"
