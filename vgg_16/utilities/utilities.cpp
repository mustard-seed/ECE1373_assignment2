#include "utilities/utilities.hpp"


void util_computeKernel(t_conv (partialOutputBuffer&)[NUM_PARALLEL_K],
                        const t_conv (computeCache&) [NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL],
                        const t_conv (computeStream&) [NUM_PARALLEL_ONE_KERNEL]
)
{
    for (unsigned int iterOutput=0; iterOutput<NUM_PARALLEL_K; iterOutput++)
    {
        for (unsigned int iterDotProduct=0; iterDotProduct<NUM_PARALLEL_ONE_KERNEL; iterDotProduct++)
        {
            partialOutputBuffer[iterOutput]
                    += computeCache[iterOutput][iterDotProduct]
                    *computeStream[iterDotProduct];
        }
    }
}
