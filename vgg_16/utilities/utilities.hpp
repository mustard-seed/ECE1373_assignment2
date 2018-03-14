#ifndef UTILITIES_HPP
#define UTILITIES_HPP
#include "../common/types.hpp"
#include "../common/params.hpp"
void util_computeKernel(t_conv partialOutputBuffer[NUM_PARALLEL_K],
        t_conv computeCache [NUM_PARALLEL_K][NUM_PARALLEL_ONE_KERNEL],
        t_conv computeStream [NUM_PARALLEL_ONE_KERNEL]
);
#endif // UTILITIES_HPP
