#ifndef CONVLAYER_HPP
#define CONVLAYER_HPP
#include "../common/types.hpp"
#include "../common/params.hpp"
void convLayer(t_conv * mem,            // global memory pointer
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

void convLayer_LoadBroadcastBuffer (const t_conv * memInput, //global memory pointer to where to start loading inputs.
        t_conv bufferBroadcast[NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing inputs
        const int inputDMax,           // input dimensions
        const int inputWMax,           // input width
        const int inputHMax,           // input height
        const int inputDOffset     //Starting channel index
        , const unsigned int pad);
#endif // CONVLAYER_HPP
