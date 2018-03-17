#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include "accelerator/accelerator.hpp"

#define INPUT_OFFSET_INDEX 0
#define WEIGHT_OFFSET_INDEX (INPUT_OFFSET_INDEX+99)
#define BIAS_OFFSET_INDEX (WEIGHT_OFFSET_INDEX+99)
#define OUTPUT_OFFSET (BIAS_OFFSET_INDEX+1)

void prepareMemory (float (&memory) [1024]);

  int main()
{

  float memory[1024];

  prepareMemory(memory);

  float correctResult=memory[BIAS_OFFSET_INDEX];

  //Compuate correct result
  for (unsigned int iter=0; iter<99; iter++)
  {
      correctResult += iter*iter*0.1;
  }

  correctResult = correctResult > 0.0 ? correctResult : 0.0;

  layerType type = CONVLayer;
  accelerator(
              (float *)&memory[0],
              0,
              sizeof(float)*(99+1+99),
              sizeof(float)*(99),
              1,
              true,

              type,

              1,
              1,
              1,
              11,
              3,
              3,
              1,
              3,
              0
              );

  std::cout <<"Correct result is: "<<correctResult<<std::endl;
  std::cout <<"Actual result is "<<memory[OUTPUT_OFFSET]<<std::endl;

  return 0;
}


void prepareMemory (float (&memory) [1024])
{

    std::cout <<"Prepareing the input and parameters "<<std::endl;

    //Prepare bias
    memory[BIAS_OFFSET_INDEX] = 1.0;


    //Prepare parameters There is only one kernel, size is 3 by 3 by 8;
    for (unsigned int iter = 0; iter < 99; iter++)
    {
        memory [WEIGHT_OFFSET_INDEX+iter] = iter*0.1;
    }

    //Prepare inputs
    //Prepare parameters There is only one kernel, size is 3 by 3 by 8;
    for (unsigned int iter = 0; iter < 99; iter++)
    {
        memory [INPUT_OFFSET_INDEX+iter] = iter;
    }
}
