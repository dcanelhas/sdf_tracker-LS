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
class principal_components
{

  public:

  void load_dictionary(std::string &filename);
  void load_mean(std::string &filename);
  void encode(thrust::device_vector<float> &input, thrust::device_vector<float> &output);
  void compare_gold(thrust::host_vector<float> &input , thrust::host_vector<float> &output);

  void compare(thrust::host_vector<float> &input , thrust::host_vector<float> &output);
  void decode(thrust::device_vector<float> &input, thrust::device_vector<float> &output);
  bool describes_empty(thrust::device_vector<float> &input, const float threshold = 1e-6);

  void get_word_for_empty(thrust::host_vector<float> &output){output = word_for_empty;}
  void get_word_for_empty(thrust::device_vector<float> &output){output = word_for_empty;}
  //host vectors
  thrust::host_vector<float> host_weights;
  thrust::host_vector<float> host_mean;

  int descriptor_size(void) {return reduced_dim;}

  //device vectors
  thrust::device_vector<float>dev_input;
  thrust::device_vector<float>dev_weights;
  thrust::device_vector<float>dev_mean;
  thrust::device_vector<float>dev_results;

protected:
  thrust::device_vector<float>word_for_empty;
  int reduced_dim;
  int input_dim;

};
