#include "convLayer.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

void convLayer(float * mem,            // global memory pointer
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
        const float * memInput, //global memory pointer to the start of inputs
        t_conv bufferBroadcast[NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing inputs
        const int inputCMax,           // input dimensions
        const int inputWMax,           // input width
        const int inputHMax,           // input height
        const int inputCOffset,     //Starting channel index
        const unsigned int pad      //Number of padding required
        )
{
    //Number of t_conv values that can be supplied per DDR access
    const unsigned int lengthInputPacket = PORT_WIDTH_BYTE / sizeof(float);

    //Actual limit in the dimension of channels
    const int inputCMaxActual = inputCMax < inputCOffset + NUM_TILE_INPUT_CONV_Z ?
                                                                      inputCMax :
                                                                      inputCOffset + NUM_TILE_INPUT_CONV_Z;

    //Counter for the number of data written to the on-chip buffer.
    unsigned int iterMemInput = 0;

    //Partial offset in C dimension
    unsigned int partialOffsetC = inputCOffset*inputHMax*inputWMax;

    //Partial indices
    unsigned int partialDepthInputCMinor;
    unsigned int partialDepthInputH;
    unsigned int partialDepthInputW;
    unsigned int partialBankInputCMinor;
    unsigned int partialBankInputH;
    unsigned int partialBankInputW;

    //Actual max W and H that take padding into account.
    const unsigned int bufferWMax = inputWMax + 2*pad;
    const unsigned int bufferHMax = inputHMax + 2*pad;


    //Buffer to temporarily hold incoming data
    //TODO: Apply HLS partition on this array
    float bufferDDR[PORT_WIDTH_BYTE / sizeof(float)];

    for (unsigned int iterC = inputCOffset, iterCMinor = 0;
         iterC < inputCMaxActual;
         iterC++, iterCMinor++)
    {
        partialDepthInputCMinor = (iterCMinor >> EXP_TILE_INPUT_CONV_Z)
                << (EXP_DEPTH_INPUT_CONV_X + EXP_DEPTH_INPUT_CONV_Y);
        partialBankInputCMinor = (iterCMinor & MASK_TILE_INPUT_CONV_Z)
               << (EXP_TILE_INPUT_CONV_X + EXP_TILE_INPUT_CONV_Y);



        //Padding at the top and the bottom
        for (unsigned iterBufferW = 0; iterBufferW <bufferWMax; iterBufferW++ )
        {
            partialDepthInputW = (iterBufferW >> EXP_TILE_INPUT_CONV_X);
            partialBankInputW = (iterBufferW & MASK_TILE_INPUT_CONV_X);

            for (unsigned iterBufferH =0; iterBufferH < pad; iterBufferH++)
            {
                partialDepthInputH = (iterBufferH >> EXP_TILE_INPUT_CONV_Y)
                         << EXP_DEPTH_INPUT_CONV_X;
                partialBankInputH = (iterBufferH & MASK_TILE_INPUT_CONV_Y)
                         << NUM_TILE_INPUT_CONV_X;

                 bufferBroadcast[partialBankInputCMinor + partialBankInputH + partialBankInputW]
                    [partialDepthInputCMinor + partialDepthInputH + partialDepthInputW]
                    = 0;
            }
            for (unsigned iterBufferH =inputHMax+pad; iterBufferH < bufferHMax; iterBufferH++)
            {
                partialDepthInputH = (iterBufferH >> EXP_TILE_INPUT_CONV_Y)
                        << EXP_DEPTH_INPUT_CONV_X;
                partialBankInputH = (iterBufferH & MASK_TILE_INPUT_CONV_Y)
                        << EXP_TILE_INPUT_CONV_X;

                 bufferBroadcast[partialBankInputCMinor + partialBankInputH + partialBankInputW]
                    [partialDepthInputCMinor + partialDepthInputH + partialDepthInputW]
                    = 0;
            }
        }


        //Padding at the left and the right
        for (unsigned iterBufferH =0; iterBufferH < bufferHMax; iterBufferH++)
        {
            partialDepthInputH = (iterBufferH >> EXP_TILE_INPUT_CONV_Y) *
                    NUM_DEPTH_INPUT_CONV_X;
            partialBankInputH = (iterBufferH & MASK_TILE_INPUT_CONV_Y)
                    *  NUM_TILE_INPUT_CONV_X;

            for (unsigned iterBufferW = 0; iterBufferW <pad; iterBufferW++ )
            {
                partialDepthInputW = (iterBufferW >> EXP_TILE_INPUT_CONV_X);
                partialBankInputW = (iterBufferW & MASK_TILE_INPUT_CONV_X);

                bufferBroadcast[partialBankInputCMinor + partialBankInputH + partialBankInputW]
                        [partialDepthInputCMinor + partialDepthInputH + partialDepthInputW]
                        = 0;

            }

            for (unsigned iterBufferW = inputWMax+pad; iterBufferW <bufferWMax; iterBufferW++ )
            {
                partialDepthInputW = (iterBufferW >> EXP_TILE_INPUT_CONV_X);
                partialBankInputW = (iterBufferW & MASK_TILE_INPUT_CONV_X);

                bufferBroadcast[partialBankInputCMinor + partialBankInputH + partialBankInputW]
                        [partialDepthInputCMinor + partialDepthInputH + partialDepthInputW]
                        = 0;

            }
        }

        //Load data from DDR
        for (unsigned int iterInputH = 0; iterInputH < inputHMax; iterInputH++)
        {
            partialDepthInputH = ( (iterInputH + pad) >> EXP_TILE_INPUT_CONV_Y)
                   << EXP_DEPTH_INPUT_CONV_X;
            partialBankInputH = ( (iterInputH + pad) & MASK_TILE_INPUT_CONV_Y)
                    << EXP_TILE_INPUT_CONV_X;

            for (unsigned int iterInputW = 0; iterInputW < inputWMax; iterInputW += lengthInputPacket)
            {
                //Read in multiple bytes from the DDR into the DDR buffer
                //TODO: verify the address is correct.
                memcpy((float *)&bufferDDR[0], (float *)(memInput + partialOffsetC + iterMemInput), PORT_WIDTH_BYTE);

                //Write data from DDR to the on-chip memory
                for (unsigned int iter=0; iter<lengthInputPacket; iter++)
                {
                    if (iterInputW + iter < inputWMax)
                    {
                        partialDepthInputW = ( (iterInputW + iter+ pad) >> EXP_TILE_INPUT_CONV_X);
                        partialBankInputW = ( (iterInputW + iter + pad) & MASK_TILE_INPUT_CONV_X);

                        bufferBroadcast[partialBankInputCMinor + partialBankInputH + partialBankInputW]
                                [partialDepthInputCMinor + partialDepthInputH + partialDepthInputW]
                                = (t_conv) bufferDDR[iter];
                        iterMemInput++;
//                        std::cout <<"Readling"<<std::endl;
//                        std::cout <<"Value "<<bufferDDR[iter]<<std::endl;
//                        std::cout <<"BANK, Depth"<<partialBankInputCMinor + partialBankInputH + partialBankInputW<<" "
//                                 <<partialDepthInputCMinor + partialDepthInputH + partialDepthInputW<<std::endl;
                    }
                }

            }
        }
    }
}

void convLayer_OffloadOutputBuffer (
        float * memOutput, //global memory pointer to the start of outputs
        const t_conv bufferOutput [NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing partial sums

        const int outputKMax,   //Output number of filters
        const int outputMMax,  //Output height
        const int outputNMax, //output width
        const int outputKOffset, //Offset of filter index

        const bool useReLu //Indicate whether ReLu is required.

        )
{
    //Burst write buffer
    float writeBuffer[PORT_WIDTH_BYTE / sizeof(t_conv)];

    //Burst write length
    const unsigned int packetLength = PORT_WIDTH_BYTE / sizeof(float);

    //Partial  Indices
    unsigned int partialBankK;
    unsigned int partialBankM;
    unsigned int partialBankN;
    unsigned int partialDepthK;
    unsigned int partialDepthM;
    unsigned int partialDepthN;

    //Partial offset relative to the start of output in global memory due to K
    unsigned int partialOffsetK = outputKOffset*outputMMax*outputNMax;

    //Output memory counter
    unsigned int memCounter = 0;

    //Need to take into account of going out of bounds
    unsigned int actualOutputKMax = outputKOffset + NUM_TILE_OUTPUT_CONV_Z < outputKMax ?
                outputKOffset + NUM_TILE_OUTPUT_CONV_Z : outputKMax;

    for (unsigned int iterOutputK = outputKOffset, iterOutputKMinor = 0;
         iterOutputK < actualOutputKMax; iterOutputK++, iterOutputKMinor++ )
    {
        partialBankK = (iterOutputKMinor & MASK_TILE_OUTPUT_CONV_Z)
                << (EXP_TILE_OUTPUT_CONV_X + EXP_TILE_OUTPUT_CONV_Y);
        partialDepthK = (iterOutputKMinor >> EXP_TILE_INPUT_CONV_Z)
                << (EXP_DEPTH_OUTPUT_CONV_X + EXP_DEPTH_OUTPUT_CONV_Y);

        for (unsigned int iterOutputM = 0; iterOutputM < outputMMax; iterOutputM++)
        {
            partialBankM = (iterOutputM & MASK_TILE_OUTPUT_CONV_Y)
                    << (EXP_TILE_OUTPUT_CONV_X);
            partialDepthM = (iterOutputM >> EXP_TILE_INPUT_CONV_Y)
                    << (EXP_DEPTH_OUTPUT_CONV_X);

            for (unsigned int iterOutputN = 0; iterOutputN < outputNMax; iterOutputN += packetLength)
            {
                for (unsigned int iter=0; iter < packetLength; iter++)
                {
                    unsigned int actualIterOutputN = iterOutputN + iter;
                    if (actualIterOutputN < outputNMax)
                    {
                        partialBankN = (actualIterOutputN & MASK_TILE_OUTPUT_CONV_X);
                        partialDepthN = actualIterOutputN >> EXP_TILE_OUTPUT_CONV_X;
                        if (useReLu)
                        {
                            writeBuffer[iter] =
                                    std::max ((float)0.0f, (float) bufferOutput[partialBankK+partialBankM+partialBankN]
                                    [partialDepthK+partialDepthM+partialDepthN]);
//                            std::cout <<"Writing"<<std::endl;
//                            std::cout <<"Value "<<writeBuffer[iter]<<std::endl;
//                            std::cout <<"BANK, Depth"<<partialBankK+partialBankM+partialBankN<<" "
//                                     <<partialDepthK+partialDepthM+partialDepthN<<std::endl;
                        }
                        else
                        {
                            writeBuffer[iter] =
                                   (float) bufferOutput[partialBankK+partialBankM+partialBankN]
                                    [partialDepthK+partialDepthM+partialDepthN];
//                            std::cout <<"Writing"<<std::endl;
//                            std::cout <<"Value "<<(float) bufferOutput[partialBankK+partialBankM+partialBankN]
//                                        [partialDepthK+partialDepthM+partialDepthN] <<std::endl;
//                            std::cout <<"BANK, Depth"<<partialBankK+partialBankM+partialBankN<<" "
//                                     <<partialDepthK+partialDepthM+partialDepthN<<std::endl;


                        }
                    }
                }
                if (iterOutputN < outputNMax - packetLength)
                {
                    //Memcopy, make sure the endianess is correct
                    memcpy((float*)(memOutput+partialOffsetK+memCounter), (float*)&writeBuffer[0], PORT_WIDTH_BYTE);
                    memCounter += packetLength;
                }
                else
                {
                    for (unsigned int iter=0; iter+iterOutputN < outputNMax; iter++)
                    {
                        *(memOutput+partialOffsetK+memCounter) = writeBuffer[iter];
                        memCounter++;
                    }
                }
            }
        }
    }
}

void convLayer_LoadWeights (
        const float *memInput, //point to start of the weights in golbal memory
        t_conv bufferWeights[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer
        const int weightRMax, //Height of kernel
        const int weightSMax, //Width of kernel
        const int weightCMax, //Number of channel per kernl
        const int weightKMax, //Number of kernels
        const int weightCOffset, //starting offset in C dimension
        const int weightKOffset //starting offset in K dimension
        )
{

    float bufferDDR[PORT_WIDTH_BYTE / sizeof(float)];
    unsigned int packetLength = PORT_WIDTH_BYTE / sizeof(float);

    const unsigned int partialOffsetWeightK = weightCMax*weightRMax*weightSMax;
    const unsigned int partialOffsetWeightC = weightRMax*weightSMax;
    const unsigned int partialOffsetWeightR = weightSMax;

    const unsigned int partialOffsetBufferC = NUM_PARALLEL_Y*NUM_PARALLEL_X;
    const unsigned int partialOffsetBufferR = NUM_PARALLEL_X;

    for (unsigned int iterBufferK=0, partialIndexWeightK=weightKOffset*partialOffsetWeightK;
         iterBufferK < NUM_PARALLEL_K;
         iterBufferK++, partialIndexWeightK += partialOffsetWeightK)
    {
        for (unsigned int iterBufferC=0, partialIndexWeightC = weightCOffset*partialOffsetWeightC, partialIndexBufferC=0;
             iterBufferC < NUM_PARALLEL_C;
             iterBufferC++, partialIndexWeightC += partialOffsetWeightC, partialIndexBufferC += partialOffsetBufferC)
        {
            for (unsigned int iterBufferR=0, partialIndexWeightR=0, partialIndexBufferR=0;
                 iterBufferR < NUM_PARALLEL_Y;
                 iterBufferR++, partialIndexWeightR += partialOffsetWeightR, partialIndexBufferR+=partialOffsetBufferR)
            {
                for (unsigned int partialIndexBufferS=0, partialIndexWeightS = 0;
                     partialIndexBufferS < NUM_PARALLEL_X;
                     partialIndexBufferS += packetLength, partialIndexWeightS += packetLength)
                {
                    memcpy((float *)&bufferDDR[0],
                            (float *)(memInput+partialIndexWeightK+partialIndexWeightC+partialIndexWeightR+partialIndexWeightS),
                            PORT_WIDTH_BYTE);
                    for (unsigned int iter=0; iter<packetLength; iter++)
                    {
                        if (iter+partialIndexBufferS < weightSMax && iterBufferR < weightRMax && iterBufferC+weightCOffset < weightCMax
                                && iterBufferK+weightKOffset < weightKMax)
                        {
                            bufferWeights[iterBufferK][partialIndexBufferS+partialIndexBufferR+partialIndexBufferC]
                                    = (t_conv)bufferDDR[iter];
                        }
                        else if (iter+partialIndexBufferS < NUM_PARALLEL_X)
                        {
                            bufferWeights[iterBufferK][partialIndexBufferS+partialIndexBufferR+partialIndexBufferC]
                                    = (t_conv)0;
                        }
                    }
                }
            }
        }
    }
}

void convLayer_prepareOutputBuffer (
        const float *memInput, //pointer to the start of biases
        t_conv bufferOutput [NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing partial sums

        const int outputKMax,   //Output number of filters
        const int outputMMax,  //Output height
        const int outputNMax, //output width
        const int outputKOffset //Offset of filter index
        )
{
    unsigned int partialBankOutputK;
    unsigned int partialBankOutputM;
     unsigned int partialBankOutputN;

     unsigned int partialDepthOutputK;
     unsigned int partialDepthOutputM;
     unsigned int partialDepthOutputN;

    for (unsigned int iterInputKMinor = 0, iterInputK = outputKOffset;
         iterInputKMinor < NUM_TILE_INPUT_CONV_Z && iterInputK < outputKMax;
         iterInputKMinor++, iterInputK++)
    {
        t_conv bias = (t_conv)*(memInput+iterInputK);

        partialBankOutputK = (iterInputKMinor & MASK_TILE_OUTPUT_CONV_Z)
                << (EXP_TILE_OUTPUT_CONV_X + EXP_TILE_OUTPUT_CONV_Y);
        partialDepthOutputK = (iterInputKMinor >> EXP_TILE_INPUT_CONV_Z)
                << (EXP_DEPTH_OUTPUT_CONV_X+EXP_DEPTH_OUTPUT_CONV_Y);

        for (unsigned int iterM = 0; iterM < outputMMax; iterM++)
        {
            partialBankOutputM = (iterM & MASK_TILE_OUTPUT_CONV_Y)
                    << (EXP_TILE_OUTPUT_CONV_X);
            partialDepthOutputM = (iterM >> EXP_TILE_INPUT_CONV_Y)
                    << (EXP_DEPTH_OUTPUT_CONV_X);
            for (unsigned int iterN = 0; iterN < outputNMax; iterN++)
            {
                partialBankOutputN = (iterN & MASK_TILE_OUTPUT_CONV_X);
                partialDepthOutputN = (iterN >> EXP_TILE_INPUT_CONV_X);
                bufferOutput[partialBankOutputK+partialBankOutputM+partialBankOutputN]
                        [partialDepthOutputK+partialDepthOutputM+partialDepthOutputN]
                        = bias;
            }
        }
    }
}
