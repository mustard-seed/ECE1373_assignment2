#include "convLayer/convLayer.hpp"
#include "accelerator/accelerator.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>
#include "utilities/utilities.hpp"

//using namespace hls;

namespace hls {

void convLayer_Forward(float * mem,            // global memory pointer
                const unsigned int inputByteOffset,       // offset of inputs in BYTES
                const unsigned int outputByteOffset,      // offset of outputs in BYTES
                const unsigned int parametersByteOffset,  // offset of parameters in BYTES
                t_conv (&bufferBroadcast)[NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing inputs
                t_conv (&bufferOutput) [NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing partial sums
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
                )
{

    const float * pointerBias = (float *)(mem + parametersByteOffset/sizeof(float));
   const unsigned int offsetWeight = parametersByteOffset/sizeof(float) + k;

    for (unsigned int iterBatch = 0; iterBatch < batchSize; iterBatch++)
    {
#ifndef __SYNTHESIS__
        std::cout <<"Working on batch index "<<iterBatch<<std::endl;
#endif
        for (unsigned int iterK = 0;
             iterK < k;
             iterK += NUM_PARALLEL_K)
        {
#ifndef __SYNTHESIS__
            std::cout <<"Working on kernel "<<iterK<<std::endl;
#endif
            hls::convLayer_PrepareOutputBuffer(
                        pointerBias,
                        bufferOutput,
                        k,
                        m,
                        n,
                        iterK
                        );
            for (unsigned int iterC=0; iterC < c; iterC++)
            {
                hls::convLayer_WrapperLoadWeightsAndInputs(
                            mem,
                            offsetWeight,
                            inputByteOffset/sizeof(float) + iterBatch*h*w*c,
                            bufferBroadcast,
                            bufferWeights,
                            c,
                            w,
                            h,
                            iterC,
                            pad,
                            kernelSize,
                            kernelSize,
                            k,
                            iterK
                            );

                hls::convLayer_ComputePartialSum(
                            bufferBroadcast,
                            bufferWeights,
                            bufferOutput,
                            m,
                            n,
                            kernelSize,
                            kernelSize,
                            c,
                            k,
                            iterC,
                            iterK,
                            stride
                            );
            }

            hls::convLayer_OffloadOutputBuffer(
                        (float *)(mem + outputByteOffset/sizeof(float) + iterBatch*m*n*k),
                        bufferOutput,
                        k,
                        m,
                        n,
                        iterK,
                        useReLu
                        );
        }
    }
}

void convLayer_LoadBroadcastBuffer (const float *memInput, //global memory pointer to where to start loading inputs.
        t_conv (&bufferBroadcast)[NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing inputs
        const unsigned int inputCMax,           // input dimensions
        const unsigned int inputWMax,           // input width
        const unsigned int inputHMax,           // input height
        const unsigned int inputCOffset     //Starting channel index
        , const unsigned int pad)
{
    //Number of t_conv values that can be supplied per DDR access
    const unsigned int lengthInputPacket = PORT_WIDTH_BYTE / sizeof(float);

    //Actual limit in the dimension of channels
    const unsigned int inputCMaxActual = inputCMax < inputCOffset + NUM_TILE_INPUT_CONV_Z ?
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
        const t_conv (&bufferOutput) [NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing partial sums

        const unsigned int outputKMax,   //Output number of filters
        const unsigned int outputMMax,  //Output height
        const unsigned int outputNMax, //output width
        const unsigned int outputKOffset, //Offset of filter index

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
         t_conv (&bufferWeights)[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer
        const unsigned int weightRMax, //Height of kernel
        const unsigned int weightSMax, //Width of kernel
        const unsigned int weightCMax, //Number of channel per kernl
        const unsigned int weightKMax, //Number of kernels
        const unsigned int weightCOffset, //starting offset in C dimension
        const unsigned int weightKOffset //starting offset in K dimension
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

void convLayer_WrapperLoadWeightsAndInputs(
        const float * mem, //global memory pointer
        const unsigned int memoryWeightoffset, //Starting FLOAT index of weights relative to mem
        const unsigned int memoryInputoffset, //starting FLOAD index of inputs relative to mem
        t_conv (&bufferBroadcast)[NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing inputs
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
        )
{
    convLayer_LoadWeights(
                (const float *)(mem+memoryWeightoffset),
                bufferWeights,
                weightRMax,
                weightSMax,
                inputCMax,
                weightKMax,
                inputCOffset,
                weightKOffset
               );

   convLayer_LoadBroadcastBuffer(
                (const float*)(mem+memoryInputoffset),
                bufferBroadcast,
                inputCMax,
                inputWMax,
                inputHMax,
                inputCOffset,
                inputPad
                );
}
void convLayer_PrepareOutputBuffer (
        const float *memInput, //pointer to the start of biases
        t_conv (&bufferOutput) [NUM_TILE_OUTPUT][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing partial sums

        const unsigned int outputKMax,   //Output number of filters
        const unsigned int outputMMax,  //Output height
        const unsigned int outputNMax, //output width
        const unsigned int outputKOffset //Offset of filter index
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

void convLayer_ComputePartialSum (
        const t_conv (&bufferBroadcast)[NUM_TILE_OUTPUT][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing inputs
        const t_conv (&bufferWeights)[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer
        t_conv (&bufferOutput) [NUM_TILE_OUTPUT][NUM_DEPTH_BROADCAST], //Array of on-chip buffer storing partial sums

        const unsigned int outputMMax,  //Output height
        const unsigned int outputNMax, //output width

        const unsigned int weightRMax, //Height of kernel
        const unsigned int weightSMax, //Width of kernel
        const unsigned int weightCMax, //Number of channel per kernl
        const unsigned int weightKMax, //Number of kernels
        const unsigned int weightCOffset, //starting offset in C dimension
        const unsigned int weightKOffset, //starting offset in K dimension
        const unsigned int stride //Stride
        )
{
    t_conv bufferInputCompute [NUM_PARALLEL_ONE_KERNEL];
    t_conv bufferOutputCompute [NUM_PARALLEL_K];

    unsigned int partialBankC;
    unsigned int partialBankH;
    unsigned int partialBankW;
    unsigned int partialDepthC;
    unsigned int partialDepthH;
    unsigned int partialDepthW;

    unsigned int partialBankK;
    unsigned int partialBankM;
    unsigned int partialBankN;
    unsigned int partialDepthK;
    unsigned int partialDepthM;
    unsigned int partialDepthN;

    for (unsigned int iterM=0, iterH=0; iterM < outputMMax; iterM++, iterH+=stride)
    {
        partialBankM = (iterM & MASK_TILE_OUTPUT_CONV_Y) << (EXP_TILE_OUTPUT_CONV_X);
        partialDepthM = (iterM >> EXP_TILE_OUTPUT_CONV_Y) << (EXP_DEPTH_OUTPUT_CONV_X);
        for (unsigned int iterN=0, iterW=0; iterN < outputNMax; iterN++, iterW+=stride)
        {
            partialBankN = (iterM & MASK_TILE_OUTPUT_CONV_X);
            partialDepthN = (iterM >> EXP_TILE_OUTPUT_CONV_X);
            unsigned int bufferInputIndex = 0;
            unsigned int bufferOutputIndex = 0;
            //Fetch inputs
            for (unsigned int iterC=0;
                 iterC < NUM_PARALLEL_C;
                 iterC++)
            {
                partialDepthC = (iterC >> EXP_TILE_INPUT_CONV_Z)
                        << (EXP_DEPTH_INPUT_CONV_X + EXP_DEPTH_INPUT_CONV_Y);
                partialBankC = (iterC & MASK_TILE_INPUT_CONV_Z)
                        << (EXP_TILE_INPUT_CONV_X + EXP_TILE_INPUT_CONV_Y);

                for (unsigned int iterR=0; iterR < NUM_PARALLEL_Y; iterR++)
                {
                    partialDepthH = ( (iterR+iterH) >> EXP_TILE_INPUT_CONV_Y)
                            << (EXP_DEPTH_INPUT_CONV_X);
                    partialBankH = ( (iterR+iterH) & MASK_TILE_INPUT_CONV_Y)
                            << (EXP_TILE_INPUT_CONV_X);
                    for (unsigned int iterS=0; iterS < NUM_PARALLEL_X; iterS++)
                    {
                        partialDepthW = ( (iterS+iterW) >> EXP_TILE_INPUT_CONV_X);
                        partialBankW = ( (iterS+iterW) & MASK_TILE_INPUT_CONV_X);

                        if (iterS < weightSMax && iterR < weightRMax && iterC+weightCOffset < weightCMax)
                        {
                            bufferInputCompute[bufferInputIndex]
                                    = bufferBroadcast[partialBankC+partialBankW+partialBankH]
                                    [partialDepthC+partialDepthH+partialDepthW];
                        }
                        else
                        {
                            bufferInputCompute[bufferInputIndex] = 0;
                        }
                        bufferInputIndex++;
                    }
                }
            }


            //Fetch existing partial sums
            for (unsigned int iterK=0; iterK < NUM_PARALLEL_K; iterK++)
            {
                partialDepthK = (iterK >> EXP_TILE_OUTPUT_CONV_Z)
                        << (EXP_DEPTH_OUTPUT_CONV_X + EXP_DEPTH_OUTPUT_CONV_Y);
                partialBankK = (iterK & MASK_TILE_OUTPUT_CONV_Z)
                        << (EXP_TILE_OUTPUT_CONV_X + EXP_TILE_OUTPUT_CONV_Y);
                if (iterK+weightKOffset<weightKMax)
                {
                    bufferOutputCompute[bufferOutputIndex]
                            = bufferOutput[partialBankK+partialBankM+partialBankN]
                            [partialDepthK+partialDepthM+partialDepthN];
                }
                else
                {
                     bufferOutputCompute[bufferOutputIndex] = 0;
                }
                bufferOutputIndex++;
            }

            //Compute partial sums
            util_computeKernel(bufferOutputCompute, bufferWeights, bufferInputCompute);

            //Write the partial sums back to output buffer
            bufferOutputIndex = 0;
            for (unsigned int iterK=0; iterK < NUM_PARALLEL_K; iterK++)
            {
                partialDepthK = (iterK >> EXP_TILE_OUTPUT_CONV_Z)
                        << (EXP_DEPTH_OUTPUT_CONV_X + EXP_DEPTH_OUTPUT_CONV_Y);
                partialBankK = (iterK & MASK_TILE_OUTPUT_CONV_Z)
                        << (EXP_TILE_OUTPUT_CONV_X + EXP_TILE_OUTPUT_CONV_Y);
                if (iterK+weightKOffset<weightKMax)
                {
                     bufferOutput[partialBankK+partialBankM+partialBankN]
                         [partialDepthK+partialDepthM+partialDepthN] = bufferOutputCompute[bufferOutputIndex];
                }
                bufferOutputIndex++;
            }
        }
    }
}
}
