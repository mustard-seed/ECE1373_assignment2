#include "accelerator/accelerator.hpp"

//#define PRINT

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

        )
{
#pragma HLS INTERFACE m_axi depth=2147483648 port=mem
#pragma HLS INTERFACE s_axilite port=inputByteOffset bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=outputByteOffset bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=parametersByteOffset bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=batchSize bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=useReLu bundle=CTRL_BUS

#pragma HLS INTERFACE s_axilite port=type bundle=CTRL_BUS

#pragma HLS INTERFACE s_axilite port=k bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=n bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=m bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=c bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=w bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=h bundle=CTRL_BUS

#pragma HLS INTERFACE s_axilite port=stride bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=kernelSize bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=pad bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS


    //ON CHIP BUFFERS

    //bufferBroadcast. Stores inputs during CONV and POOL operation. Store weights during FC operation
    //t_conv bufferBroadcast[NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST];

   // t_conv bufferBroadcastB[NUM_INPUT_Z][NUM_INPUT_Y][NUM_INPUT_X];

    //bufferOutput
    t_conv bufferOutput[NUM_OUTPUT_Z][NUM_OUTPUT_Y][NUM_OUTPUT_X];
 //Array of on-chip buffer storing partial sums

    //bufferCache. Stores weights during CONV and POOL operations. Stores inputs during FC operations
   // t_conv bufferCacheA[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL];
   // t_conv bufferCacheB[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL];
 //weight buffer

    //t_conv (*p_bufferBroadcast)[NUM_DEPTH_BROADCAST] = bufferBroadcast;
    switch (type)
    {
        case (CONVLayer):
           convLayer_Forward(
                        mem,
                        inputByteOffset,
                        outputByteOffset,
                        parametersByteOffset,
						//bufferBroadcastA,
						//bufferBroadcastB,
                        bufferOutput,
                       // bufferCacheA,
						//bufferCacheB,
                        batchSize,
                        k,
                        n,
                        m,
                        c,
                        w,
                        h,
                        stride,
                        kernelSize,
                        pad,
                        useReLu
                        );
            break;
        case (FCLayer):
            break;
        case(POOLLayer):
            break;
        default:
            break;
    }
}


void util_computeKernel(
        t_conv (&partialOutputBuffer)[NUM_PARALLEL_K],
        const t_conv (&computeCache) [NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL],
       const t_conv (&computeStream) [NUM_PARALLEL_ONE_KERNEL]
)
{

	COMPUTE_FOR_OUTPUT:
	for (unsigned int iterOutput=0; iterOutput<NUM_PARALLEL_K; iterOutput++)
    {
		COMPUTE_FOR_DOT_PRODUCT:
        for (unsigned int iterDotProduct=0; iterDotProduct<NUM_PARALLEL_ONE_KERNEL; iterDotProduct++)
        {
        	partialOutputBuffer[iterOutput]
                    += computeCache[iterOutput][iterDotProduct]
                    *computeStream[iterDotProduct];
        }
    }
}

void convLayer_Forward(float * mem,            // global memory pointer
                const unsigned int inputByteOffset,       // offset of inputs in BYTES
                const unsigned int outputByteOffset,      // offset of outputs in BYTES
                const unsigned int parametersByteOffset,  // offset of parameters in BYTES
                //t_conv (&bufferBroadcastA)[NUM_INPUT_Z][NUM_INPUT_Y][NUM_INPUT_X], //Array of on-chip buffer storing inputs
				//t_conv (&bufferBroadcastB)[NUM_INPUT_Z][NUM_INPUT_Y][NUM_INPUT_X], //Array of on-chip buffer storing inputs
                t_conv (&bufferOutput) [NUM_OUTPUT_Z][NUM_OUTPUT_Y][NUM_OUTPUT_X], //Array of on-chip buffer storing partial sums
                //t_conv (&bufferWeightsA)[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer
				//t_conv (&bufferWeightsB)[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer
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

#pragma HLS INLINE

    const float * pointerBias = (float *)(mem + parametersByteOffset/SIZE_OF_FLOAT + k*kernelSize*kernelSize*c);
   const unsigned int offsetWeight = parametersByteOffset/SIZE_OF_FLOAT;

   const unsigned int inputOffsetIterBatchConstant = h*w*c;
   const unsigned int outputOffsetIterBatchConstant = m*n*k;

   CONVLAYER_FORWARD_FOR_BATCH:
    for (unsigned int iterBatch = 0, inputPartialIndexIterBatch = 0, outputPartialIndexBatch = 0;
         iterBatch < batchSize;
         iterBatch++,  inputPartialIndexIterBatch+=inputOffsetIterBatchConstant , outputPartialIndexBatch += outputOffsetIterBatchConstant)
    {
#ifndef __SYNTHESIS__
#ifdef PRINT
        std::cout <<"Working on batch index "<<iterBatch<<std::endl;
#endif
#endif
        CONVLAYER_FORWARD_FOR_K:
        for (unsigned int iterK = 0;
             iterK < k;
             iterK += NUM_PARALLEL_K)
        {
#ifndef __SYNTHESIS__
#ifdef PRINT
            std::cout <<"Working on kernel "<<iterK<<std::endl;
#endif
#endif
           convLayer_PrepareOutputBuffer(
                        pointerBias,
                        bufferOutput,
                        k,
                        m,
                        n,
                        iterK
                        );
           convLayer_ComputeWrapper(
        		   mem,
        	inputByteOffset,
			inputPartialIndexIterBatch,
			//bufferBroadcastA,
			bufferOutput,
			//bufferWeightsA,
			iterK,
			offsetWeight,
			k,
			n,
			m,
			c,
			w,
			h,
			stride,
			kernelSize,
			pad
           );


            convLayer_OffloadOutputBuffer(
                        (float *)(mem + outputByteOffset/SIZE_OF_FLOAT + outputPartialIndexBatch),
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
        t_conv (&bufferBroadcast)[NUM_INPUT_Z][NUM_INPUT_Y][NUM_INPUT_X], //Array of on-chip buffer storing inputs
        const unsigned int inputCMax,           // input dimensions
        const unsigned int inputWMax,           // input width
        const unsigned int inputHMax,           // input height
        const unsigned int inputCOffset     //Starting channel index
        , const unsigned int pad)
{
#pragma HLS INLINE
    //Number of t_conv values that can be supplied per DDR access
    const unsigned int lengthInputPacket = PORT_WIDTH_BYTE / SIZE_OF_FLOAT;

    //Actual limit in the dimension of channels
    const unsigned int inputCMaxActual = inputCMax < inputCOffset + NUM_INPUT_Z?
                                                                      inputCMax :
                                                                      inputCOffset + NUM_INPUT_Z;

    //Counter for the number of data written to the on-chip buffer.
    unsigned int iterMemInput = 0;

    //Partial offset in C dimension
    const unsigned int partialOffsetC = inputCOffset*inputHMax*inputWMax;

    //Partial indices
//    unsigned int partialDepthInputCMinor;
//    unsigned int partialDepthInputH;
//    unsigned int partialDepthInputW;
//    unsigned int partialBankInputCMinor;
//    unsigned int partialBankInputH;
//    unsigned int partialBankInputW;

    //Actual max W and H that take padding into account.
    const unsigned int bufferWMax = inputWMax + pad + pad;
    const unsigned int bufferHMax = inputHMax + pad + pad;


    //Buffer to temporarily hold incoming data
    //TODO: Apply HLS partition on this array
    float bufferDDR[PORT_WIDTH_BYTE / SIZE_OF_FLOAT];

    CONVLAYER_LOADBROACASTBUFFER_FOR_C:
    for (unsigned int iterC = inputCOffset, iterCMinor = 0;
         iterC < inputCMaxActual;
         iterC++, iterCMinor++)
    {
//        partialDepthInputCMinor = (iterCMinor >> EXP_TILE_INPUT_CONV_Z)
//                << (EXP_DEPTH_INPUT_CONV_X + EXP_DEPTH_INPUT_CONV_Y);
//        partialBankInputCMinor = (iterCMinor & MASK_TILE_INPUT_CONV_Z)
//               << (EXP_TILE_INPUT_CONV_X + EXP_TILE_INPUT_CONV_Y);



        //Padding at the top and the bottom
        CONVLAYER_LOADBROACASTBUFFER_FOR_PAD_TOPBOTTOM:
        for (unsigned iterBufferW = 0; iterBufferW <bufferWMax; iterBufferW++ )
        {
//            partialDepthInputW = (iterBufferW >> EXP_TILE_INPUT_CONV_X);
//            partialBankInputW = (iterBufferW & MASK_TILE_INPUT_CONV_X);

            convLayer_LoadBroadcastBuffer_label1:
            for (unsigned iterBufferH =0; iterBufferH < pad; iterBufferH++)
            {
//                partialDepthInputH = (iterBufferH >> EXP_TILE_INPUT_CONV_Y)
//                         << EXP_DEPTH_INPUT_CONV_X;
//                partialBankInputH = (iterBufferH & MASK_TILE_INPUT_CONV_Y)
//                         << NUM_TILE_INPUT_CONV_X;

                 bufferBroadcast[iterCMinor][iterBufferH][iterBufferW]
                    = 0;
            }
            convLayer_LoadBroadcastBuffer_label2:
            for (unsigned iterBufferH =inputHMax+pad; iterBufferH < bufferHMax; iterBufferH++)
            {
//                partialDepthInputH = (iterBufferH >> EXP_TILE_INPUT_CONV_Y)
//                        << EXP_DEPTH_INPUT_CONV_X;
//                partialBankInputH = (iterBufferH & MASK_TILE_INPUT_CONV_Y)
//                        << EXP_TILE_INPUT_CONV_X;

                bufferBroadcast[iterCMinor][iterBufferH][iterBufferW]
                   = 0;
            }
        }


        //Padding at the left and the right
        CONVLAYER_LOADBROACASTBUFFER_FOR_PAD_LEFTRIGHT:
        for (unsigned iterBufferH =0; iterBufferH < bufferHMax; iterBufferH++)
        {
//            partialDepthInputH = (iterBufferH >> EXP_TILE_INPUT_CONV_Y) *
//                    NUM_DEPTH_INPUT_CONV_X;
//            partialBankInputH = (iterBufferH & MASK_TILE_INPUT_CONV_Y)
//                    *  NUM_TILE_INPUT_CONV_X;

            convLayer_LoadBroadcastBuffer_label3:
            for (unsigned iterBufferW = 0; iterBufferW <pad; iterBufferW++ )
            {
//                partialDepthInputW = (iterBufferW >> EXP_TILE_INPUT_CONV_X);
//                partialBankInputW = (iterBufferW & MASK_TILE_INPUT_CONV_X);

                bufferBroadcast[iterCMinor][iterBufferH][iterBufferW]
                   = 0;

            }

        convLayer_LoadBroadcastBuffer_label4:
        for (unsigned iterBufferW = inputWMax+pad; iterBufferW <bufferWMax; iterBufferW++ )
            {
//                partialDepthInputW = (iterBufferW >> EXP_TILE_INPUT_CONV_X);
//                partialBankInputW = (iterBufferW & MASK_TILE_INPUT_CONV_X);

            bufferBroadcast[iterCMinor][iterBufferH][iterBufferW]
               = 0;

            }
        }

        //Load data from DDR
        CONVLAYER_LOADBROACASTBUFFER_FOR_LOAD_H:
        for (unsigned int iterInputH = 0; iterInputH < inputHMax; iterInputH++)
        {
//            partialDepthInputH = ( (iterInputH + pad) >> EXP_TILE_INPUT_CONV_Y)
//                   << EXP_DEPTH_INPUT_CONV_X;
//            partialBankInputH = ( (iterInputH + pad) & MASK_TILE_INPUT_CONV_Y)
//                    << EXP_TILE_INPUT_CONV_X;

            CONVLAYER_LOADBROACASTBUFFER_FOR_LOAD_W:
            for (unsigned int iterInputW = 0; iterInputW < inputWMax; iterInputW += lengthInputPacket)
            {

                //Read in multiple bytes from the DDR into the DDR buffer
                //TODO: verify the address is correct.
                memcpy((float *)&bufferDDR[0], (float *)(memInput + partialOffsetC + iterMemInput), PORT_WIDTH_BYTE);

                convLayer_LoadBroadcastBuffer_label0:
                for (unsigned int iter=0; iter < lengthInputPacket; iter++)
                {
                    if (iter + iterInputW < inputWMax)
                    {
                        bufferBroadcast[iterCMinor][iterInputH+pad][iter+iterInputW+pad] = (t_conv) bufferDDR[iter];
                        iterMemInput++;
                     }
                }
             }

//                //Write data from DDR to the on-chip memory
//                CONVLAYER_LOADBROACASTBUFFER_FOR_LOAD_PACKET:
//                for (unsigned int iter=0; iter<lengthInputPacket; iter++)
//                {
//                    if (iterInputW + iter < inputWMax)
//                    {
//                        partialDepthInputW = ( (iterInputW + iter+ pad) >> EXP_TILE_INPUT_CONV_X);
//                        partialBankInputW = ( (iterInputW + iter + pad) & MASK_TILE_INPUT_CONV_X);

//                        bufferBroadcast[partialBankInputCMinor + partialBankInputH + partialBankInputW]
//                                [partialDepthInputCMinor + partialDepthInputH + partialDepthInputW]
//                                = (t_conv) bufferDDR[iter];
//                        iterMemInput++;
////                        std::cout <<"Readling"<<std::endl;
////                        std::cout <<"Value "<<bufferDDR[iter]<<std::endl;
////                        std::cout <<"BANK, Depth"<<partialBankInputCMinor + partialBankInputH + partialBankInputW<<" "
////                                 <<partialDepthInputCMinor + partialDepthInputH + partialDepthInputW<<std::endl;
//                    }
//                }

            }
        }

}

void convLayer_OffloadOutputBuffer (
        float * memOutput, //global memory pointer to the start of outputs
        const t_conv (&bufferOutput) [NUM_OUTPUT_Z][NUM_OUTPUT_Y][NUM_OUTPUT_X], //Array of on-chip buffer storing partial sums

        const unsigned int outputKMax,   //Output number of filters
        const unsigned int outputMMax,  //Output height
        const unsigned int outputNMax, //output width
        const unsigned int outputKOffset, //Offset of filter index

        const bool useReLu //Indicate whether ReLu is required.

        )
{
#pragma HLS INLINE
    //Burst write buffer
    float writeBuffer[PORT_WIDTH_BYTE / SIZE_OF_FLOAT];

    //Burst write length
    const unsigned int packetLength = PORT_WIDTH_BYTE / SIZE_OF_FLOAT;

    //Partial  Indices
//    unsigned int partialBankK;
//    unsigned int partialBankM;
//    unsigned int partialBankN;
//    unsigned int partialDepthK;
//    unsigned int partialDepthM;
//    unsigned int partialDepthN;

    //Partial offset relative to the start of output in global memory due to K
    unsigned int partialOffsetK = outputKOffset*outputMMax*outputNMax;

    //Output memory counter
    unsigned int memCounter = 0;

    //Need to take into account of going out of bounds
    unsigned int actualOutputKMax = outputKOffset + NUM_PARALLEL_K < outputKMax ?
                outputKOffset + NUM_PARALLEL_K : outputKMax;

    CONVLAYER_OFFLOADRESULT_FOR_K:
    for (unsigned int iterOutputK = outputKOffset, iterOutputKMinor = 0;
         iterOutputK < actualOutputKMax; iterOutputK++, iterOutputKMinor++ )
    {
//        partialBankK = (iterOutputKMinor & MASK_TILE_OUTPUT_CONV_Z)
//                << (EXP_TILE_OUTPUT_CONV_X + EXP_TILE_OUTPUT_CONV_Y);
//        partialDepthK = (iterOutputKMinor >> EXP_TILE_INPUT_CONV_Z)
//                << (EXP_DEPTH_OUTPUT_CONV_X + EXP_DEPTH_OUTPUT_CONV_Y);

        CONVLAYER_OFFLOADRESULT_FOR_M:
        for (unsigned int iterOutputM = 0; iterOutputM < outputMMax; iterOutputM++)
        {
//            partialBankM = (iterOutputM & MASK_TILE_OUTPUT_CONV_Y)
//                    << (EXP_TILE_OUTPUT_CONV_X);
//            partialDepthM = (iterOutputM >> EXP_TILE_OUTPUT_CONV_Y)
//                    << (EXP_DEPTH_OUTPUT_CONV_X);

            CONVLAYER_OFFLOADRESULT_FOR_N:
            for (unsigned int iterOutputN = 0; iterOutputN < outputNMax; iterOutputN += packetLength)
            {
            	CONVLAYER_OFFLOADRESULT_FOR_PACKET:
                for (unsigned int iter=0; iter < packetLength; iter++)
                {
                    unsigned int actualIterOutputN = iterOutputN + iter;
                    if (actualIterOutputN < outputNMax)
                    {
//                        partialBankN = (actualIterOutputN & MASK_TILE_OUTPUT_CONV_X);
//                        partialDepthN = actualIterOutputN >> EXP_TILE_OUTPUT_CONV_X;
                        if (useReLu)
                        {
                            writeBuffer[iter] =
                                    std::max ((float)0.0f, (float) bufferOutput[iterOutputKMinor][iterOutputM][actualIterOutputN]);
                            //writeBuffer[iter] = partialOffsetK+memCounter+iter;
                                    //
//                            std::cout <<"Writing"<<std::endl;
//                            std::cout <<"Value "<<writeBuffer[iter]<<std::endl;
//                            std::cout <<"BANK, Depth"<<partialBankK+partialBankM+partialBankN<<" "
//                                     <<partialDepthK+partialDepthM+partialDepthN<<std::endl;
                        }
                        else
                        {
                            writeBuffer[iter] =
                                   (float) bufferOutput[iterOutputKMinor][iterOutputM][actualIterOutputN];
//                            std::cout <<"Writing"<<std::endl;
//                            std::cout <<"Value "<<(float) bufferOutput[partialBankK+partialBankM+partialBankN]
//                                        [partialDepthK+partialDepthM+partialDepthN] <<std::endl;
//                            std::cout <<"BANK, Depth"<<partialBankK+partialBankM+partialBankN<<" "
//                                     <<partialDepthK+partialDepthM+partialDepthN<<std::endl;
                        }
                    }
                }
//                for (unsigned int iter=0; iter < packetLength; iter++)
//				{
//                	if (iter+iterOutputN < outputNMax)
//                	{
//                		*(memOutput+partialOffsetK+memCounter) = writeBuffer[iter];
//                		memCounter++;
//                	}
//				}
                if (iterOutputN < outputNMax - packetLength)
                {
                    memcpy((float *)(memOutput+partialOffsetK+memCounter), (float *)&writeBuffer[0], PORT_WIDTH_BYTE);
                    memCounter += packetLength;
                }
                else
                {
                    convLayer_OffloadOutputBuffer_label0:
					for (unsigned int iter=0; iter<packetLength; iter++)
                    {
                        if (iter + iterOutputN < outputNMax)
                        {
                             *(memOutput+partialOffsetK+memCounter) = writeBuffer[iter];
                                memCounter++;
                        }
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
#pragma HLS INLINE

    float bufferDDR[PORT_WIDTH_BYTE / SIZE_OF_FLOAT];
    unsigned int packetLength = PORT_WIDTH_BYTE / SIZE_OF_FLOAT;

    const unsigned int partialOffsetWeightK = weightCMax*weightRMax*weightSMax;
    const unsigned int partialOffsetWeightC = weightRMax*weightSMax;
    const unsigned int partialOffsetWeightR = weightSMax;

    const unsigned int partialOffsetBufferC = NUM_PARALLEL_Y*NUM_PARALLEL_X;
    const unsigned int partialOffsetBufferR = NUM_PARALLEL_X;


    CONVLAYER_LOADWEIGHTS_FOR_PARALLEL_K:
    for (unsigned int iterBufferK=0, partialIndexWeightK=weightKOffset*partialOffsetWeightK;
         iterBufferK < NUM_PARALLEL_K;
         iterBufferK++, partialIndexWeightK += partialOffsetWeightK)
    {
    	CONVLAYER_LOADWEIGHTS_FOR_PARALLEL_C:
        for (unsigned int iterBufferC=0, partialIndexWeightC = weightCOffset*partialOffsetWeightC, partialIndexBufferC=0;
             iterBufferC < NUM_PARALLEL_C;
             iterBufferC++, partialIndexWeightC += partialOffsetWeightC, partialIndexBufferC += partialOffsetBufferC)
        {
        	CONVLAYER_LOADWEIGHTS_FOR_PARALLEL_R:
            for (unsigned int iterBufferR=0, partialIndexWeightR=0, partialIndexBufferR=0;
                 iterBufferR < NUM_PARALLEL_Y;
                 iterBufferR++, partialIndexWeightR += partialOffsetWeightR, partialIndexBufferR+=partialOffsetBufferR)
            {
            	CONVLAYER_LOADWEIGHTS_FOR_PARALLEL_S:
                for (unsigned int partialIndexBufferS=0, partialIndexWeightS = 0;
                     partialIndexBufferS < NUM_PARALLEL_X;
                     partialIndexBufferS += packetLength, partialIndexWeightS += packetLength)
                {

                	memcpy((float *)&bufferDDR[0], (float *)(memInput+partialIndexWeightK+partialIndexWeightC+partialIndexWeightR+partialIndexWeightS),
                            PORT_WIDTH_BYTE);

                    convLayer_LoadWeights_label1:
					for (unsigned int iter=0; iter<NUM_PARALLEL_X; iter++)
                    {
						assert(iterBufferK < NUM_PARALLEL_K);
					    assert(iter+partialIndexBufferS+partialIndexBufferR+partialIndexBufferC < NUM_PARALLEL_ONE_KERNEL);
                        if (iter+partialIndexBufferS < weightSMax && iterBufferR < weightRMax && iterBufferC+weightCOffset < weightCMax
                                && iterBufferK+weightKOffset < weightKMax)
                        {

                            bufferWeights[iterBufferK][iter+partialIndexBufferS+partialIndexBufferR+partialIndexBufferC]
                                    = (t_conv)bufferDDR[iter];
                        }
                        else if (iter+partialIndexBufferS < NUM_PARALLEL_X)
                        {
                            bufferWeights[iterBufferK][iter+partialIndexBufferS+partialIndexBufferR+partialIndexBufferC]
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
        const float * &memInput, //pointer to the start of biases
        t_conv (&bufferOutput) [NUM_OUTPUT_Z][NUM_OUTPUT_Y][NUM_OUTPUT_X], //Array of on-chip buffer storing partial sums

        const unsigned int outputKMax,   //Output number of filters
        const unsigned int outputMMax,  //Output height
        const unsigned int outputNMax, //output width
        const unsigned int outputKOffset //Offset of filter index
        )
{
#pragma HLS INLINE
//    unsigned int partialBankOutputK;
//    unsigned int partialBankOutputM;
//     unsigned int partialBankOutputN;

//     unsigned int partialDepthOutputK;
//     unsigned int partialDepthOutputM;
//     unsigned int partialDepthOutputN;

    CONVLAYER_PREPAREOUTPUT_FOR_K:
    for (unsigned int iterInputKMinor = 0, iterInputK = outputKOffset;
         iterInputKMinor < NUM_OUTPUT_Z && iterInputK < outputKMax;
         iterInputKMinor++, iterInputK++)
    {
        t_conv bias = (t_conv) *(memInput+iterInputK);

//        partialBankOutputK = (iterInputKMinor & MASK_TILE_OUTPUT_CONV_Z)
//                << (EXP_TILE_OUTPUT_CONV_X + EXP_TILE_OUTPUT_CONV_Y);
//        partialDepthOutputK = (iterInputKMinor >> EXP_TILE_OUTPUT_CONV_Z)
//                << (EXP_DEPTH_OUTPUT_CONV_X+EXP_DEPTH_OUTPUT_CONV_Y);

        CONVLAYER_PREPAREOUTPUT_FOR_M:
        for (unsigned int iterM = 0; iterM < outputMMax; iterM++)
        {
//            partialBankOutputM = (iterM & MASK_TILE_OUTPUT_CONV_Y)
//                    << (EXP_TILE_OUTPUT_CONV_X);
//            partialDepthOutputM = (iterM >> EXP_TILE_OUTPUT_CONV_Y)
//                    << (EXP_DEPTH_OUTPUT_CONV_X);
            CONVLAYER_PREPAREOUTPUT_FOR_N:
            for (unsigned int iterN = 0; iterN < outputNMax; iterN++)
            {
//                partialBankOutputN = (iterN & MASK_TILE_OUTPUT_CONV_X);
//                partialDepthOutputN = (iterN >> EXP_TILE_INPUT_CONV_X);
                bufferOutput[iterInputKMinor][iterM][iterN]
                        = bias;
//                 bufferOutput[iterInputKMinor][iterM][iterN] = iterInputK*outputMMax*outputNMax + iterM*outputNMax + iterN;
            }
        }
    }
}

void convLayer_ComputePartialSum (
        const t_conv (&bufferBroadcast)[NUM_INPUT_Z][NUM_INPUT_Y][NUM_INPUT_X], //Array of on-chip buffer storing inputs
        const t_conv (&bufferWeights)[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer
        t_conv (&bufferOutput) [NUM_OUTPUT_Z][NUM_OUTPUT_Y][NUM_OUTPUT_X], //Array of on-chip buffer storing partial sums

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

//    unsigned int partialBankC;
//    unsigned int partialBankH;
//    unsigned int partialBankW;
//    unsigned int partialDepthC;
//    unsigned int partialDepthH;
//    unsigned int partialDepthW;

//    unsigned int partialBankK;
//    unsigned int partialBankM;
//    unsigned int partialBankN;
//    unsigned int partialDepthK;
//    unsigned int partialDepthM;
//    unsigned int partialDepthN;

    CONVLAYER_COMPUTE_FOR_M:
    for (unsigned int iterM=0, iterH=0; iterM < outputMMax; iterM++, iterH+=stride)
    {
//        partialBankM = (iterM & MASK_TILE_OUTPUT_CONV_Y) << (EXP_TILE_OUTPUT_CONV_X);
//        partialDepthM = (iterM >> EXP_TILE_OUTPUT_CONV_Y) << (EXP_DEPTH_OUTPUT_CONV_X);
        CONVLAYER_COMPUTE_FOR_N:
        for (unsigned int iterN=0, iterW=0; iterN < outputNMax; iterN++, iterW+=stride)
        {
//            partialBankN = (iterM & MASK_TILE_OUTPUT_CONV_X);
//            partialDepthN = (iterM >> EXP_TILE_OUTPUT_CONV_X);
            unsigned int bufferInputIndex = 0;
            unsigned int bufferOutputIndex = 0;
            //Fetch inputs
            CONVLAYER_COMPUTE_FOR_INPUTBUFFER_C:
            for (unsigned int iterC=0;
                 iterC < NUM_PARALLEL_C;
                 iterC++)
            {
//                partialDepthC = (iterC >> EXP_TILE_INPUT_CONV_Z)
//                        << (EXP_DEPTH_INPUT_CONV_X + EXP_DEPTH_INPUT_CONV_Y);
//                partialBankC = (iterC & MASK_TILE_INPUT_CONV_Z)
//                        << (EXP_TILE_INPUT_CONV_X + EXP_TILE_INPUT_CONV_Y);

                CONVLAYER_COMPUTE_FOR_INPUTBUFFER_R:
                for (unsigned int iterR=0; iterR < NUM_PARALLEL_Y; iterR++)
                {
//                    partialDepthH = ( (iterR+iterH) >> EXP_TILE_INPUT_CONV_Y)
//                            << (EXP_DEPTH_INPUT_CONV_X);
//                    partialBankH = ( (iterR+iterH) & MASK_TILE_INPUT_CONV_Y)
//                            << (EXP_TILE_INPUT_CONV_X);
                    CONVLAYER_COMPUTE_FOR_INPUTBUFFER_S:
                    for (unsigned int iterS=0; iterS < NUM_PARALLEL_X; iterS++)
                    {
//                        partialDepthW = ( (iterS+iterW) >> EXP_TILE_INPUT_CONV_X);
//                        partialBankW = ( (iterS+iterW) & MASK_TILE_INPUT_CONV_X);

                        if (iterS < weightSMax && iterR < weightRMax && iterC+weightCOffset < weightCMax)
                        {
                            bufferInputCompute[bufferInputIndex]
                                    = bufferBroadcast[iterC][iterR+iterH][iterS+iterW];
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
            CONVLAYER_COMPUTE_FOR_OUTPUTBUFFER_K:
            for (unsigned int iterK=0; iterK < NUM_PARALLEL_K; iterK++)
            {
//                partialDepthK = (iterK >> EXP_TILE_OUTPUT_CONV_Z)
//                        << (EXP_DEPTH_OUTPUT_CONV_X + EXP_DEPTH_OUTPUT_CONV_Y);
//                partialBankK = (iterK & MASK_TILE_OUTPUT_CONV_Z)
//                        << (EXP_TILE_OUTPUT_CONV_X + EXP_TILE_OUTPUT_CONV_Y);
                if (iterK+weightKOffset<weightKMax)
                {
                    bufferOutputCompute[bufferOutputIndex]
                            = bufferOutput[iterK][iterM][iterN];
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
            CONVLAYER_COMPUTE_FOR_OFFLOAD_K:
            for (unsigned int iterK=0; iterK < NUM_PARALLEL_K; iterK++)
            {
//                partialDepthK = (iterK >> EXP_TILE_OUTPUT_CONV_Z)
//                        << (EXP_DEPTH_OUTPUT_CONV_X + EXP_DEPTH_OUTPUT_CONV_Y);
//                partialBankK = (iterK & MASK_TILE_OUTPUT_CONV_Z)
//                        << (EXP_TILE_OUTPUT_CONV_X + EXP_TILE_OUTPUT_CONV_Y);
                if (iterK+weightKOffset<weightKMax)
                {
                     bufferOutput[iterK]
                         [iterM][iterN] = bufferOutputCompute[bufferOutputIndex];
                }
                bufferOutputIndex++;
            }
        }
    }
}

void convLayer_ComputeWrapper(float * mem,            // global memory pointer
                const unsigned int inputByteOffset,       // offset of inputs in BYTES
                const unsigned int inputPartialIndexIterBatch,      // offset of input due to batches in INDEX
               // t_conv (&bufferBroadcastA)[NUM_INPUT_Z][NUM_INPUT_Y][NUM_INPUT_X], //Array of on-chip buffer storing inputs
				//t_conv (&bufferBroadcastB)[NUM_INPUT_Z][NUM_INPUT_Y][NUM_INPUT_X], //Array of on-chip buffer storing inputs
                t_conv (&bufferOutput) [NUM_OUTPUT_Z][NUM_OUTPUT_Y][NUM_OUTPUT_X], //Array of on-chip buffer storing partial sums
               // t_conv (&bufferWeightsA)[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer
				//t_conv (&bufferWeightsB)[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL], //weight buffer
				const unsigned int iterK,            // current C index
				const unsigned int offsetWeight,
                const unsigned int k,           // output number of kernels
                const unsigned int n,           // output width
                const unsigned int m,           // output height
                const unsigned int c,           // input dimensions
                const unsigned int w,           // input width
                const unsigned int h,           // input height
                const unsigned int stride,            // stride
                const unsigned int kernelSize ,        // kernel size
                const unsigned int pad //pad size
                )
{
	 t_conv bufferBroadcastA[NUM_INPUT_Z][NUM_INPUT_Y][NUM_INPUT_X];

	 //bufferCache. Stores weights during CONV and POOL operations. Stores inputs during FC operations
	 t_conv bufferWeightsA[NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL];
	 CONVLAYER_FORWARD_FOR_C:
	for (unsigned int iterC=0; iterC < c; iterC+=NUM_PARALLEL_C)
	            {

						convLayer_WrapperLoadWeightsAndInputs(
									mem,
									offsetWeight,
									inputByteOffset/SIZE_OF_FLOAT + inputPartialIndexIterBatch,
									bufferBroadcastA,
									bufferWeightsA,
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

					   convLayer_ComputePartialSum(
									bufferBroadcastA,
									bufferWeightsA,
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

}

