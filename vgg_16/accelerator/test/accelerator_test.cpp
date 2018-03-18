#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include "shared/shared.hpp"
#include "accelerator/accelerator.hpp"
#include <sstream>
#include <chrono>

#define HW_CTRL_ADDR 0x00000000

using namespace std;

#define MAX_KERNEL_SIZE 3
#define MAX_INPUT_DIMS 512
#define MAX_OUTPUT_DIMS 512
#define MAX_INPUT_WIDTH 226
#define MAX_INPUT_HEIGHT 226
#define MAX_OUTPUT_WIDTH 224
#define MAX_OUTPUT_HEIGHT 224
#define MAX_CONV_INPUT MAX_INPUT_DIMS*MAX_INPUT_WIDTH*MAX_INPUT_HEIGHT
#define MAX_CONV_OUTPUT MAX_OUTPUT_DIMS*MAX_OUTPUT_WIDTH*MAX_OUTPUT_HEIGHT
#define MAX_WEIGHT_SIZE MAX_OUTPUT_DIMS*MAX_INPUT_DIMS*MAX_KERNEL_SIZE*MAX_KERNEL_SIZE

#define PRINT

int run_single_test(string imageDir, map<string, int> layer_params, float * &dma_input, float * gold_outputs, bool runOnFPGA){




  int num_outputs = layer_params["output_dim"]*layer_params["output_width"]*
                    layer_params["output_height"];
  int num_inputs = layer_params["input_dim"]*layer_params["input_width"]*
                   layer_params["input_height"];
  int num_weights = layer_params["input_dim"]*layer_params["output_dim"]*
                    layer_params["kernel_size"]*layer_params["kernel_size"];
  int num_biases = layer_params["output_dim"];

  // very basic input checking
  if (layer_params["input_dim"] > MAX_INPUT_DIMS ||
      layer_params["input_width"] > MAX_INPUT_WIDTH ||
      layer_params["input_height"] > MAX_INPUT_WIDTH ||
      layer_params["output_dim"] > MAX_OUTPUT_DIMS ||
      layer_params["output_width"] > MAX_OUTPUT_WIDTH ||
      layer_params["output_height"] > MAX_OUTPUT_WIDTH ||
      layer_params["batch_size"] > MAX_BATCH)
  {
    cerr << "Problem with layer params\n";
    return 1;
      }
  else {
        int b = layer_params["batch_size"];
        int od = layer_params["output_dim"];
        int ox = layer_params["output_width"];
        int oy = layer_params["output_height"];
        int id = layer_params["input_dim"];
        int ix = layer_params["input_width"];
        int iy = layer_params["input_height"];
        int k = layer_params["kernel_size"];
        int s = layer_params["stride"];
        int pad = layer_params["pad"];

    #ifdef PRINT
        cout << "Begin Test\n"
           << "Batch Size: " << b << endl
           << "Num Inputs: " << num_inputs << endl
           << "Num Outputs: " << num_outputs << endl
           << "Num Weights: " << num_weights << endl
           << "Num Biases: " << num_biases << endl
           << "Input Dimensions " << b << " x " << id << " x " << ix << " x " << iy << endl
           << "Output Dimensions " << b << " x " << od << " x " << ox << " x " << oy << endl
           << "Kernel Dimensions " << od << " x " << id << " x " << k << " x " << k << endl
           << "Stride Size: " << s << endl;
    #endif

        layerType type = CONVLayer;
        // Run Accelerator
        if (runOnFPGA)
        {
             hw_accelerator(HW_CTRL_ADDR,
                           dma_input,
                           sizeof(float)*(num_biases + num_weights),
                           sizeof(float)*(b*num_inputs+num_biases + num_weights),
                           0,
                           b,
                           true,

                           type,

                           od,
                           ox,
                           oy,
                           id,
                           ix,
                           iy,s,
                           k,
                           0    //only for the first layer, because the input is already padded.
                           );
        }
        else
        {

            accelerator(
                    dma_input,
                    sizeof(float)*(num_biases + num_weights),
                    sizeof(float)*(b*num_inputs+num_biases + num_weights),
                    0,
                    b,
                    true,

                    type,

                    od,
                    ox,
                    oy,
                    id,
                    ix,
                    iy,s,
                    k,
                    0    //only for the first layer, because the input is already padded.
                    );
      }
    }
  return 0;

}


int main(int argc, char *argv[])
{

    bool runOnFPGA;
  switch(* (argv[1]))
  {
    case ('7'):
      std::cout <<"Running on FPGA!!!!!!"<<std::endl;
      runOnFPGA = true;
      break;
     default:
      std::cout <<"Running on CPU"<<std::endl;
      runOnFPGA = false;
  }

  string imageRootDir = "data/vgg_batches/batch_";
  int numBatches = 10;
  string layer = "conv1_1";
  string imageDir;
  ostringstream ss;
  float total_error = 0.0;
  cout << "Reading Input for " << numBatches << " batches" <<  endl;

  vector<map<string, int> > batch_layer_params = readBatchParams(imageRootDir, numBatches, layer);
  vector<float *> dma_input_vec;
  vector<float *> gold_outputs_vec;
  if(readInputBatches(imageRootDir, batch_layer_params, numBatches, layer, MAX_WEIGHT_SIZE+MAX_OUTPUT_DIMS+MAX_BATCH*MAX_CONV_INPUT+MAX_BATCH*MAX_CONV_OUTPUT, dma_input_vec, CONV))
    return 1;
  if(readOutputBatches(imageRootDir, batch_layer_params, numBatches, layer, MAX_BATCH*MAX_CONV_OUTPUT, gold_outputs_vec, CONV)) return 1;


  cout << "Starting Test with " << numBatches << " batches" <<  endl;


  auto start = chrono::system_clock::now();
  for(int i=0; i<numBatches; i++){
    ss << i;
#ifdef PRINT
    cout << "Running batch" << i << endl;
#endif
    imageDir = imageRootDir + ss.str() + "/" + layer;

    if(run_single_test(imageDir, batch_layer_params[i], dma_input_vec[i], gold_outputs_vec[i], runOnFPGA)!=0)
    return 1;
  }
  auto end = chrono::system_clock::now();
  auto elapsed = end - start;

  float avg_error = get_mean_squared_error_and_write_file(dma_input_vec, gold_outputs_vec, numBatches, batch_layer_params, imageRootDir, layer, CONV);

  cout << "Mean Square Error " << avg_error << endl;
  cout << "Computation took  " << chrono::duration_cast<chrono::seconds> (elapsed).count() << " seconds" << endl;
  std::cout << "DONE" << std::endl;
  return 0;
}
