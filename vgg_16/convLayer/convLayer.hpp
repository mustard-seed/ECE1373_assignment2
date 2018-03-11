#ifndef CONVLAYER_HPP
#define CONVLAYER_HPP
#include "../common/types.hpp"
void conv_layer(t_conv * mem,            // global memory pointer
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

#endif // CONVLAYER_HPP
