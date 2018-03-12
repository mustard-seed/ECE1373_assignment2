#include "convLayer.hpp"
#include <cstring>

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
                const int k)            // kernel size
{

// Global memory interface
#pragma HLS INTERFACE m_axi port=mem depth=2147483648
// Bind all control ports to a single bundle
#pragma HLS INTERFACE s_axilite port=b bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=od bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=ox bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=oy bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=id bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=ix bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=iy bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=s bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=k bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=input_offset
#pragma HLS INTERFACE s_axilite port=output_offset
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS


}

void convLayer_LoadBroadcastBuffer (
        const t_conv * memInput, //global memory pointer to where to start loading inputs.
        t_conv bufferBroadcast[NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing inputs
        const int inputDMax,           // input dimensions
        const int inputWMax,           // input width
        const int inputHMax,           // input height
        const int inputDOffset,     //Starting channel index
        const unsigned int pad      //Number of padding required
        )
{
    //Number of t_conv values that can be supplied per DDR access
    const unsigned int lengthInputPacket = PORT_WIDTH_BYTE / sizeof(t_conv);

    //Actual limit in the dimension of channels
    const int inputDMaxActual = inputDMax < inputDOffset + 0x1 << EXP_TILE_INPUT_CONV_Z ?
                                                                      inputDmax :
                                                                      inputDOffset + 0x1 << EXP_TILE_INPUT_CONV_Z;

    //Counter for the number of data written to the on-chip buffer.
    unsigned int iterMemInput = 0;

    unsigned int offsetDepthInputDepth = 0;
    unsigned int offsetDepthInputHeight = 0;
    unsigned int offsetBankInputDepth = 0;
    unsigned int offsetBankInputHeight = 0;


    //Buffer to temporarily hold incoming data
    //TODO: Apply HLS partition on this array
    t_conv bufferDDR[PORT_WIDTH_BYTE / sizeof(t_conv)];

    for (unsigned int iterD = inputDOffset, iterDMinor = 0;
         iterD < inputDMaxActual;
         iterD++, iterDMinor++)
    {
        for (unsigned int iterH = 0; iterH < inputHMax; iterH++)
        {
            for (unsigned int iterW = 0; iterW < inputWMax; iterW += lengthInputPacket)
            {
                if ()
                //Read in multiple bytes from the DDR into the DDR buffer
                memcpy((void *)&bufferDDR[0], (void *)memInput, PORT_WIDTH_BYTE);

                //Write data from DDR to the on-chip memory
                for (unsigned int iter=0; iter<lengthInputPacket; iter++)
                {
                    if (iterW + iter < inputMax)
                    {
                        bufferBroadcast[];
                        iterMemInput++;
                    }
                }

            }
            offsetDepthInputHeight += (iterH >> EXP_TILE_INPUT_CONV_Y) *
                    NUM_DEPTH_INPUT_CONV_X;
            offsetBankInputHeight += (iterH & MASK_TILE_INPUT_CONV_Y)
                    * ( (0x1) << (EXP_TILE_INPUT_CONV_X) );
        }

        offsetDepthInputDepth += (iterD >> EXP_TILE_INPUT_CONV_Z) *
                NUM_DEPTH_INPUT_CONV_X * NUM_DEPTH_INPUT_CONV_Y;
        offsetBankInputDepth += (iterD & MASK_TILE_INPUT_CONV_Z)
                * ( (0x1) << (EXP_TILE_INPUT_CONV_X + EXP_TILE_INPUT_CONV_Y));
    }
}
