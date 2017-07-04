#include <thrust/device_vector.h>

#ifndef TSDF_BLOCKSIZE 
  #define TSDF_BLOCKSIZE 16
#endif

#ifndef TSDF_TRUNC_PLUS
  #define TSDF_TRUNC_PLUS 0.1
#endif

#ifndef TSDF_TRUNC_NEG
  #define TSDF_TRUNC_NEG -0.04
#endif

enum activation_fcn {TANH, SIGMOID};

class neural_network
{


  public:

  void load_network(const std::string &directory=".", int layers=10);
  void compare(thrust::host_vector<float> &input , thrust::host_vector<float> &output);
  void compare_gold(thrust::host_vector<float> &input , thrust::host_vector<float> &output);
  void encode(thrust::device_vector<float> &input, thrust::device_vector<float> &output);
  void decode(thrust::device_vector<float> &input, thrust::device_vector<float> &output, enum activation_fcn = SIGMOID);
  bool describes_empty(thrust::device_vector<float> &input, float threshold = 0.1);

  int descriptor_size(void) {return reduced_dim;}

  //host vectors
  thrust::host_vector<float> host_weights[20];
  thrust::host_vector<float> host_biases[20];

  //device vectors
  thrust::device_vector<float>dev_input;
  thrust::device_vector<float>dev_weights[20];
  thrust::device_vector<float>dev_biases[20];
  thrust::device_vector<float>dev_results[20];
  
  protected:
  thrust::host_vector<int> network_structure;
  int reduced_dim;
  int num_layers;
  thrust::device_vector<float>word_for_empty;

};

