#include "../convLayer.hpp"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <time.h>       /* time */

#define TEST_DATAFLOW_INPUT_X 5
#define TEST_DATAFLOW_INPUT_Y 5
#define TEST_DATAFLOW_INPUT_Z 4
#define TEST_DATAFLOW_INPUT_TOTAL (TEST_DATAFLOW_INPUT_X*TEST_DATAFLOW_INPUT_Y*TEST_DATAFLOW_INPUT_Z)

#define PAD 1

#define TEST_DATAFLOW_OUTPUT_X (TEST_DATAFLOW_INPUT_X+2*PAD)
#define TEST_DATAFLOW_OUTPUT_Y (TEST_DATAFLOW_INPUT_Y+2*PAD)
#define TEST_DATAFLOW_OUTPUT_Z TEST_DATAFLOW_INPUT_Z
#define TEST_DATAFLOW_OUTPUT_TOTAL (TEST_DATAFLOW_OUTPUT_X*TEST_DATAFLOW_OUTPUT_Y*TEST_DATAFLOW_OUTPUT_Z)

int main()
{
    srand (time(NULL));
//    {
//        std::cout <<"Starting CONV Layer Testing" <<std::endl;
//        std::cout <<"Test DataFlow: Streaming data in and out of CONV layer"<<std::endl;

//        //Prepare some data flow input
//        float testDataFlowInput[TEST_DATAFLOW_INPUT_TOTAL];
//        float testDataFlowBuffer[NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST];
//        float testDataFlowOutputCorrect[TEST_DATAFLOW_OUTPUT_TOTAL];
//        float testDataFlowOutputActual[TEST_DATAFLOW_OUTPUT_TOTAL];

//        std::cout <<"Preparing Input data and correct output data"<< std::endl;
//        for (unsigned int iterZ=0; iterZ < TEST_DATAFLOW_OUTPUT_Z; iterZ++)
//        {
//            for (unsigned int iterY=0; iterY < TEST_DATAFLOW_OUTPUT_Y; iterY++)
//            {
//                for (unsigned int iterX=0; iterX < TEST_DATAFLOW_OUTPUT_X; iterX++)
//                {
//                    if (iterX < PAD | iterX >= PAD+TEST_DATAFLOW_INPUT_X
//                          |  iterY < PAD | iterY >= PAD+TEST_DATAFLOW_INPUT_Y)
//                    {
//                        testDataFlowOutputCorrect[iterZ*TEST_DATAFLOW_OUTPUT_X
//                                *TEST_DATAFLOW_OUTPUT_Y
//                                +iterY * TEST_DATAFLOW_OUTPUT_X
//                                +iterX] = 0.0f;
//                    }
//                    else
//                    {
//                        float val = (rand() % 10 - 5) * 1.0f;
//                        testDataFlowOutputCorrect[iterZ*TEST_DATAFLOW_OUTPUT_X
//                                *TEST_DATAFLOW_OUTPUT_Y
//                                +iterY * TEST_DATAFLOW_OUTPUT_X
//                                +iterX] = val;
//                        testDataFlowInput[iterZ*TEST_DATAFLOW_INPUT_X
//                                *TEST_DATAFLOW_INPUT_Y
//                                +(iterY-PAD) * TEST_DATAFLOW_INPUT_X
//                                +(iterX-PAD)] = val;
//                    }
//                }
//            }
//        }

//        std::cout <<"Conducting test"<<std::endl;
//        for (unsigned int iterZ=0; iterZ < TEST_DATAFLOW_INPUT_Z; iterZ += NUM_TILE_INPUT_CONV_Z)
//        {
//            convLayer_LoadBroadcastBuffer(&testDataFlowInput[0],
//                    testDataFlowBuffer, TEST_DATAFLOW_INPUT_Z, TEST_DATAFLOW_INPUT_X, TEST_DATAFLOW_INPUT_Y,
//                    iterZ*TEST_DATAFLOW_INPUT_X*TEST_DATAFLOW_INPUT_Y, PAD);
//            convLayer_OffloadOutputBuffer(&testDataFlowOutputActual[0], testDataFlowBuffer
//                    ,TEST_DATAFLOW_OUTPUT_Z, TEST_DATAFLOW_OUTPUT_Y, TEST_DATAFLOW_OUTPUT_X,
//                    iterZ*TEST_DATAFLOW_OUTPUT_X*TEST_DATAFLOW_OUTPUT_Y, false);
//        }
//        std::cout <<"Calculating error"<<std::endl;
//        float squaredError = 0.0f;
//        for (unsigned int iter=0; iter<TEST_DATAFLOW_OUTPUT_TOTAL; iter++)
//        {
//            squaredError += pow(testDataFlowOutputActual[iter] - testDataFlowOutputCorrect[iter], 2.0);
//        }
//        float rms = sqrt(squaredError) / TEST_DATAFLOW_OUTPUT_TOTAL;
//        std::cout <<"RMS: "<<rms<<std::endl;
//     }

    {
        std::cout <<"Test DataFlow with Relu: Streaming data in and out of CONV layer"<<std::endl;

        //Prepare some data flow input
        float testDataFlowInput[TEST_DATAFLOW_INPUT_TOTAL];
        float testDataFlowBuffer[NUM_TILE_BROADCAST][NUM_DEPTH_BROADCAST];
        float testDataFlowOutputCorrect[TEST_DATAFLOW_OUTPUT_TOTAL];
        float testDataFlowOutputActual[TEST_DATAFLOW_OUTPUT_TOTAL];

        std::cout <<"Preparing Input data and correct output data"<< std::endl;
        for (unsigned int iterZ=0; iterZ < TEST_DATAFLOW_OUTPUT_Z; iterZ++)
        {
            for (unsigned int iterY=0; iterY < TEST_DATAFLOW_OUTPUT_Y; iterY++)
            {
                for (unsigned int iterX=0; iterX < TEST_DATAFLOW_OUTPUT_X; iterX++)
                {
                    if (iterX < PAD || iterX >= PAD+TEST_DATAFLOW_INPUT_X
                          ||  iterY < PAD || iterY >= PAD+TEST_DATAFLOW_INPUT_Y)
                    {
                        testDataFlowOutputCorrect[iterZ*TEST_DATAFLOW_OUTPUT_X
                                *TEST_DATAFLOW_OUTPUT_Y
                                +iterY * TEST_DATAFLOW_OUTPUT_X
                                +iterX] = 0.0f;
                    }
                    else
                    {
                        float val = (rand() % 10 - 5) * 1.0f;
                        testDataFlowOutputCorrect[iterZ*TEST_DATAFLOW_OUTPUT_X
                                *TEST_DATAFLOW_OUTPUT_Y
                                +iterY * TEST_DATAFLOW_OUTPUT_X
                                +iterX] = std::max(0.0f, val);
                        testDataFlowInput[iterZ*TEST_DATAFLOW_INPUT_X
                                *TEST_DATAFLOW_INPUT_Y
                                +(iterY-PAD) * TEST_DATAFLOW_INPUT_X
                                +(iterX-PAD)] = val;
                    }
                }
            }
        }

        std::cout <<"Conducting test"<<std::endl;
        for (unsigned int iterZ=0; iterZ < TEST_DATAFLOW_INPUT_Z; iterZ += NUM_TILE_INPUT_CONV_Z)
        {
            convLayer_LoadBroadcastBuffer(&testDataFlowInput[0],
                    testDataFlowBuffer, TEST_DATAFLOW_INPUT_Z, TEST_DATAFLOW_INPUT_X, TEST_DATAFLOW_INPUT_Y,
                    iterZ, PAD);
            convLayer_OffloadOutputBuffer(&testDataFlowOutputActual[0], testDataFlowBuffer
                    ,TEST_DATAFLOW_OUTPUT_Z, TEST_DATAFLOW_OUTPUT_Y, TEST_DATAFLOW_OUTPUT_X,
                    iterZ, true);
        }
        std::cout <<"Calculating error"<<std::endl;
        float squaredError = 0.0f;
        for (unsigned int iter=0; iter<TEST_DATAFLOW_OUTPUT_TOTAL; iter++)
        {
            squaredError += pow(testDataFlowOutputActual[iter] - testDataFlowOutputCorrect[iter], 2.0);
            std::cout <<"iter, actual, correct "<<iter<<" "<<testDataFlowOutputActual[iter]<<" "<<testDataFlowOutputCorrect[iter]<<std::endl;
        }
        float rms = sqrt(squaredError) / TEST_DATAFLOW_OUTPUT_TOTAL;
        std::cout <<"RMS: "<<rms<<std::endl;
     }


    return 0;
}
