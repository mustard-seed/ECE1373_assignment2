#ifndef ACCELERATOR_HPP
#define ACCELERATOR_HPP
#include "common/types.hpp"
#include "common/params.hpp"
#include <iostream>
#include <cstring>

enum layerType {CONVLayer, FCLayer, POOLLayer};

void accelerator (
        float * mem, //global memory pointer
        unsigned int inputByteOffset,       // offset of inputs in BYTES
        unsigned int outputByteOffset,      // offset of outputs in BYTES
        unsigned int parametersByteOffset,  // offset of parameters in BYTES
        const unsigned int batchSize,            // batch size
        const bool useReLu, //whether to use ReLu (ALL)

        layerType type,

        const unsigned int k,           // output number of kernels (CONV, POOL), or number of outputs (FC)
        const unsigned int n,           // output width (CONV)
        const unsigned int m,           // output height (CONV)
        const unsigned int c,           // input number of channels  (CONV, POOL), or number of inputs (FC)
        const unsigned int w,           // input width (CONV, POOL)
        const unsigned int h,           // input height (CONV, POOL)
        const unsigned int stride,            // stride (CONV, POOL)
        const unsigned int kernelSize ,        // kernel size (CONV, POOL)
        const unsigned int pad  //pad size (CONV, POOL)

        );

void util_computeKernel(
        t_conv (&partialOutputBuffer)[NUM_PARALLEL_K],
        const t_conv (&computeCache) [NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL],
       const t_conv (&computeStream) [NUM_PARALLEL_ONE_KERNEL]
);


void convLayer_Forward(float * mem,            // global memory pointer
                const unsigned int inputByteOffset,       // offset of inputs in BYTES
                const unsigned int outputByteOffset,      // offset of outputs in BYTES
                const unsigned int parametersByteOffset,  // offset of parameters in BYTES
               t_conv (&bufferBroadcast)[NUM_INPUT_Z][NUM_INPUT_Y][NUM_INPUT_X], //Array of on-chip buffer storing inputs
               t_conv (&bufferOutput) [NUM_OUTPUT_Z][NUM_OUTPUT_Y][NUM_OUTPUT_X], //Array of on-chip buffer storing partial sums
               t_conv (&bufferWeights)[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer
                const unsigned int batchSize,            // batch size
                const unsigned int k,           // output number of kernels
                const unsigned int n,           // output width
                const unsigned int m,           // output height
                const unsigned int c,           // input dimensions
                const unsigned int w,           // input width
                const unsigned int h,           // input height
                const unsigned int stride,            // stride
                const unsigned int kernelSize ,        // kernel size
                const unsigned int pad,  //pad size
                const bool useReLu //whether to use ReLu
                );

/*!
 * \brief convLayer_LoadBroadcastBuffer
 * \param memInput
 * \param bufferBroadcast
 * \param inputCMax
 * \param inputWMax
 * \param inputHMax
 * \param inputCOffset
 * \param pad
 * \details Load input activation of volume NUM_TILE_INPUT_CONV_Z * inputHMax*inputWMax to on-chip broadcast buffer.
 * Handles zero padding.
 *
 */
void convLayer_LoadBroadcastBuffer (const float *memInput, //global memory pointer to where to start loading inputs.
        t_conv (&bufferBroadcast)[NUM_INPUT_Z][NUM_INPUT_Y][NUM_INPUT_X], //Array of on-chip buffer storing inputs
        const unsigned int inputCMax,           // input dimensions
        const unsigned int inputWMax,           // input width
        const unsigned int inputHMax,           // input height
        const unsigned int inputCOffset,     //Starting channel index
        const unsigned int pad
        );

void convLayer_OffloadOutputBuffer (float * memOutput, //global memory pointer to the start of outputs
        const t_conv (&bufferOutput)[NUM_OUTPUT_Z][NUM_OUTPUT_Y][NUM_OUTPUT_X], //Array of on-chip buffer storing partial sums

        const unsigned int outputKMax,   //Output number of filters
        const unsigned int outputMMax,  //Output height
        const unsigned int outputNMax, //output width
        const unsigned int outputKOffset, //Offset of filter index

        const bool useReLu //Indicate whether ReLu is required.
                                    );

void convLayer_LoadWeights (
        const float *memInput, //point to start of the weights in golbal memory
         t_conv (&bufferWeights)[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer
        const unsigned int weightRMax, //Height of kernel
        const unsigned int weightSMax, //Width of kernel
        const unsigned int weightCMax, //Number of channel per kernl
        const unsigned int weightKMax, //Number of kernels
        const unsigned int weightCOffset, //starting offset in C dimension
        const unsigned int weightKOffset //starting offset in K dimension
        );

void convLayer_WrapperLoadWeightsAndInputs(const float * mem, //global memory pointer
        const unsigned int memoryWeightoffset, //Starting FLOAT index of weights relative to mem
        const unsigned int memoryInputoffset, //starting FLOAD index of inputs relative to mem
        t_conv (&bufferBroadcast)[NUM_INPUT_Z][NUM_INPUT_Y][NUM_INPUT_X], //Array of on-chip buffer storing inputs
        t_conv (&bufferWeights)[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer

        const unsigned int inputCMax,           // input dimensions
        const unsigned int inputWMax,           // input width
        const unsigned int inputHMax,           // input height
        const unsigned int inputCOffset,     //Starting channel index
        const unsigned int inputPad, //Number of input padding required.

        const unsigned int weightRMax, //Height of kernel
        const unsigned int weightSMax, //Width of kernel
        const unsigned int weightKMax, //Number of kernel
        const unsigned int weightKOffset //starting offset in K dimension
        );
void convLayer_PrepareOutputBuffer (const float * &memInput, //point to start loading biases
        t_conv (&bufferOutput)[NUM_OUTPUT_Z][NUM_OUTPUT_Y][NUM_OUTPUT_X], //Array of on-chip buffer storing partial sums
        const unsigned int outputKMax,   //Output number of filters
        const unsigned int outputMMax,  //Output height
        const unsigned int outputNMax, //output width
        const unsigned int outputKOffset //Offset of filter index
        );

void convLayer_ComputePartialSum (const t_conv (&bufferBroadcast)[NUM_INPUT_Z][NUM_INPUT_Y][NUM_INPUT_X], //Array of on-chip buffer storing inputs
        const t_conv (&bufferWeights)[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer
        t_conv (&bufferOutput)[NUM_OUTPUT_Z][NUM_OUTPUT_Y][NUM_OUTPUT_X], //Array of on-chip buffer storing partial sums

        const unsigned int outputMMax,  //Output height
        const unsigned int outputNMax, //output width

        const unsigned int weightRMax, //Height of kernel
        const unsigned int weightSMax, //Width of kernel
        const unsigned int weightCMax, //Number of channel per kernl
        const unsigned int weightKMax, //Number of kernels
        const unsigned int weightCOffset, //starting offset in C dimension
        const unsigned int weightKOffset, //starting offset in K dimension
        const unsigned int stride //Stride
        );
#endif // ACCELERATOR_HPP
