#ifndef CONVLAYER_HPP
#define CONVLAYER_HPP
#include "../common/types.hpp"
#include "../common/params.hpp"
void convLayer(float *mem,            // global memory pointer
                int input_offset,       // offset of inputs
                int output_offset,      // offset of outputs
                const int b,            // batch size
                const int od,           // output dimensions
                const int ox,           // output width
                const int oy,           // output height
                const int id,           // input dimensions
                const int ix,           // input width
                const int iy,           // input height
                const int s,            // stride
                const int k);           // kernel size

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
        t_conv bufferBroadcast[NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing inputs
        const int inputCMax,           // input dimensions
        const int inputWMax,           // input width
        const int inputHMax,           // input height
        const int inputCOffset     //Starting channel index
        , const unsigned int pad);

void convLayer_OffloadOutputBuffer (float * memOutput, //global memory pointer to where to start writing the outputs
        const t_conv bufferOutput [NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing partial sums

        const int outputKMax,   //Output number of filters
        const int outputMMax,  //Output height
        const int outputNMax, //output width
        const int outputKOffset, //Offset of filter index

        const bool useReLu);

void convLayer_LoadWeights (
        const float *memInput, //point to start of the weights in golbal memory
        t_conv bufferWeights[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer
        const int weightRMax, //Height of kernel
        const int weightSMax, //Width of kernel
        const int weightCMax, //Number of channel per kernl
        const int weightKMax, //Number of kernels
        const int weightCOffset, //starting offset in C dimension
        const int weightKOffset //starting offset in K dimension
        );

void convLayer_prepareOutputBuffer (
        const float *memInput, //point to start loading biases
        t_conv bufferOutput [NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing partial sums

        const int outputKMax,   //Output number of filters
        const int outputMMax,  //Output height
        const int outputNMax, //output width
        const int outputKOffset //Offset of filter index
        );
#endif // CONVLAYER_HPP
