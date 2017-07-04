#define OC_NEW_STYLE_INCLUDES 1
#include "chooseser.h"
#include "principal_components.h"
#include "vector_functions.h"


#include <thrust/transform_reduce.h>
#include <thrust/functional.h>
#include <thrust/fill.h>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/for_each.h>
#include <thrust/execution_policy.h>
#include <thrust/system_error.h>

#include <cublas_v2.h>
#include <cuda_runtime.h>
#include <string>
#include <iostream>
#include <vector>
#include <ctime>


void principal_components::load_dictionary(std::string &filename)
{
    input_dim = TSDF_BLOCKSIZE*TSDF_BLOCKSIZE*TSDF_BLOCKSIZE;
    // sprintf(weight_file, "sparse_pca_128.pickle");

    std::cout<< "Reading file: " << filename << "." <<std::endl;

    Array<real_4> w_arr;
    Val w_val; LoadValFromFile( filename.c_str() , w_val, SERIALIZE_P2); w_arr = (w_val);

//    host_weights[layer_number][ nodes[layer_number]*n_out + n_in ];

    std::cout<< "Got " << w_arr.length() <<  " weights." <<std::endl;
    reduced_dim = w_arr.length()/input_dim;

    for(int w_idx = 0; w_idx<w_arr.length(); ++w_idx )
    host_weights.push_back((w_arr[w_idx]));

    dev_weights = host_weights;

    thrust::device_vector<float> empty_data;
    empty_data.resize(input_dim);
    thrust::fill(empty_data.begin(), empty_data.end(),1.0f);
    encode(empty_data, word_for_empty);

    std::cout<< "loaded weights " << w_arr.length() <<  " weights." <<std::endl;

}

void principal_components::load_mean(std::string &filename)
{
    input_dim = TSDF_BLOCKSIZE*TSDF_BLOCKSIZE*TSDF_BLOCKSIZE;
    // sprintf(weight_file, "sparse_pca_128.pickle");

    std::cout<< "Reading file: " << filename << "." <<std::endl;

    Array<real_4> w_arr;
    Val w_val; LoadValFromFile( filename.c_str() , w_val, SERIALIZE_P2); w_arr = (w_val);

//    host_weights[layer_number][ nodes[layer_number]*n_out + n_in ];

    std::cout<< "Got " << w_arr.length() <<  " weights." <<std::endl;

    for(int w_idx = 0; w_idx<w_arr.length(); ++w_idx )
    host_mean.push_back((w_arr[w_idx]));

    dev_mean = host_mean;


}


void principal_components::encode(thrust::device_vector<float> &input, thrust::device_vector<float> &output)
{

  // cublasSgemv performs the computation y = alpha*( A*x ) + beta * y
  // since y is our bias vector, we need to make a copy so it doesn't get replaced with the
  // results of the computation. A represents the weights and x is the input to the current layer
  // After each such computation, we also have to evaluate the activation function of the network
  // This is done using thrust::for_each()

  thrust::device_vector<float> input_minus_mean;
  input_minus_mean.resize(input_dim);
  output.resize(reduced_dim);

  //find the mean
//  float mean = thrust::reduce(thrust::device, input.begin(), input.end())/float(input_dim);
//  thrust::fill(output.begin(), output.end(), mean);

  //make an input-sized vector containing the mean
  // mean_vector.resize(input_dim);
  // thrust::fill(mean_vector.begin(), mean_vector.end(), mean);

  //subtract the mean from the input to make it centered
  thrust::transform(input.begin(), input.end(), dev_mean.begin(), input_minus_mean.begin(), subtract<float>() );

  float alf = 1.0f; float beta = 0;

  //cudaStream_t Stream;
  //cudaStreamCreate(&Stream);

  cublasHandle_t handle;
  (cublasCreate(&handle));

  //cublasSetStream(handle, Stream);

  (cublasSgemv( handle, CUBLAS_OP_T, 4096, reduced_dim, &alf, (thrust::raw_pointer_cast(&dev_weights[0])), 4096,
    thrust::raw_pointer_cast(&input_minus_mean[0]), 1, &beta,thrust::raw_pointer_cast(&output[0]),1));

  (cublasDestroy(handle));
  //cudaStreamDestroy(Stream);


}


bool principal_components::describes_empty(thrust::device_vector<float> &input, const float threshold)
{
  thrust::plus<float> binary_op;
  thrust::device_vector<float> r_vec;
  r_vec.resize(reduced_dim);

  thrust::transform(input.begin(), input.end(), word_for_empty.begin(), r_vec.begin(), abs_diff<float>());
  float distance = thrust::reduce(r_vec.begin(), r_vec.end(), 0.0f, binary_op);

  return (distance < threshold) ? true : false;
}


void principal_components::decode(thrust::device_vector<float> &input, thrust::device_vector<float> &output)
{
  // cublasSgemv performs the computation y = alpha*( A*x ) + beta * y

  // output.resize(4096);
  output = dev_mean;
  // thrust::fill(output.begin(), output.end(),1.0f);
  float alf = 1.0f;
  //beta holds the mean that was extracted earlier, during encoding. it has to be added to the final result
  float beta = 1.0f;//input[reduced_dim];

  //cudaStream_t Stream;
  //cudaStreamCreate(&Stream);

  cublasHandle_t handle;
  (cublasCreate(&handle));

  //cublasSetStream(handle, Stream);

  (cublasSgemv( handle, CUBLAS_OP_N, 4096, reduced_dim, &alf, (thrust::raw_pointer_cast(&dev_weights[0])), 4096,
    thrust::raw_pointer_cast(&input[0]), 1, &beta,thrust::raw_pointer_cast(&output[0]),1));

  (cublasDestroy(handle));
  //cudaStreamDestroy(Stream);
}

void principal_components::compare(thrust::host_vector<float> &input, thrust::host_vector<float> &output)
{
  thrust::device_vector<float> original = input;
  thrust::device_vector<float> reconstructed = original;
  thrust::device_vector<float> descriptor;

  encode(original,descriptor);

//  std::cout << "encoding original " << original.size() << " elements to " << descriptor.size() << " elements." << std::endl;


  decode(descriptor,reconstructed);
//  std::cout << "decoding descriptor " << descriptor.size() << " elements to " << reconstructed.size() << " elements." << std::endl;
  output = reconstructed;
}


void principal_components::compare_gold(thrust::host_vector<float> &input ,thrust::host_vector<float> &output )
{
/*
  int nodes[num_layers+1] =     { 4096,     2048,     1024,     512 ,     256,      128,      256,      512,      1024,     2048,     4096};
  int num_outputs[num_layers] = {           nodes[1], nodes[2], nodes[3], nodes[4], nodes[5], nodes[6], nodes[7], nodes[8], nodes[9], nodes[10] };
  int num_inputs[num_layers] =  { nodes[0], nodes[1], nodes[2], nodes[3], nodes[4], nodes[5], nodes[6], nodes[7], nodes[8], nodes[9]};

  output.resize(nodes[num_layers]);

  //allocating neural network container
  float* outputs[num_layers];
  float* activations[num_layers];

  for (int layer_number = 0; layer_number < num_layers; ++layer_number)
  {
    outputs[layer_number] = new float[num_outputs[layer_number]];
    activations[layer_number] = new float[num_outputs[layer_number]];
  }

  //set activations and output to zero so we can compute the sums
  for(uint layer_number = 0; layer_number < num_layers; layer_number++)
    for (uint fill = 0; fill < num_outputs[layer_number]; ++fill)
      outputs[layer_number][fill] = activations[layer_number][fill] = 0.f;

  for(uint layer_number = 0; layer_number < num_layers; ++layer_number)
  {
    // #pragma omp parallel
    for(uint n_out = 0; n_out < num_outputs[layer_number]; ++n_out)
    {
      //compute activations layerwise
      for(uint n_in = 0; n_in < num_inputs[layer_number]; ++n_in)
      {

        activations[layer_number][n_out] += (layer_number == 0) ? (input[n_in]) * host_weights[layer_number][ nodes[layer_number]*n_out + n_in ] :
                                            outputs[layer_number-1][n_in] * host_weights[layer_number][ nodes[layer_number]*n_out + n_in ];
      }
      outputs[layer_number][n_out] = logistic(activations[layer_number][n_out] + host_biases[layer_number][n_out] );
    }
  }

  for(int d=0; d < input_dim; ++d)
  {
    output[d] = outputs[num_layers-1][d];
  }

  for (int del = 0; del < num_layers; ++del)
  {
    if(activations[del]!=NULL)
      delete[] activations[del];
    if(outputs[del]!=NULL)
      delete[] outputs[del];
  }*/
}
