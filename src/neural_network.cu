#define OC_NEW_STYLE_INCLUDES 1
#include "chooseser.h"
#include "neural_network.h"
#include "vector_functions.h"

#include <thrust/fill.h>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/for_each.h>
#include <thrust/execution_policy.h>
#include <thrust/system_error.h>

#include <math.h>
#include <cublas_v2.h>
#include <cuda_runtime.h>
#include <string>
#include <iostream>
#include <vector>
#include <ctime>


struct tanh_functor
{
  __host__ __device__
  void operator()(float &x)
  {
    x = tanh(x);
  }
};


inline float logistic(float x){ return 1.0f/(1.0f + expf(-x)); }
struct logistic_functor
{
  __host__ __device__
  void operator()(float &x)
  {
    x = 1.0f/(1.0f + expf(-x));
  }
};

void neural_network::load_network(const std::string &directory, int layers)
{  
  
  num_layers = layers;
  network_structure.push_back(TSDF_BLOCKSIZE*TSDF_BLOCKSIZE*TSDF_BLOCKSIZE);

  for (size_t i = 0; i < num_layers; ++i)
  {

    char str[15];
    sprintf(str, "%d", i+1);

    std::string weight_file = directory + "fw" + str + ".pickle" ;
    std::string bias_file = directory + "fb" + str + ".pickle" ;
    // char bias_file[50];
    // sprintf(weight_file, directory.c_str() + "fw%d.pickle", i+1);
    // sprintf(bias_file, "directory.c_str() + fb%d.pickle", i+1);

    std::cout<< "Reading files: " << weight_file << ", " << bias_file << "." <<std::endl;

    Array<real_4> w_arr;
    Array<real_4> b_arr;
    Val w_val; LoadValFromFile( weight_file.c_str() , w_val, SERIALIZE_P2); w_arr = (w_val);
    Val b_val; LoadValFromFile( bias_file.c_str(), b_val, SERIALIZE_P2); b_arr = (b_val);

    network_structure.push_back(b_arr.length());

//    host_weights[layer_number][ nodes[layer_number]*n_out + n_in ];

    std::cout<< "Got " << w_arr.length() <<  " weights and "<< b_arr.length() << " biases." <<std::endl;

    for(int w_idx = 0; w_idx<w_arr.length(); ++w_idx )
      host_weights[i].push_back((w_arr[w_idx]));

    for(int b_idx = 0; b_idx<b_arr.length(); ++b_idx )
      host_biases[i].push_back((b_arr[b_idx]));

    dev_biases[i] = host_biases[i];  dev_weights[i] = host_weights[i];
  }
  
  thrust::device_vector<float> empty_data;
  empty_data.resize(TSDF_BLOCKSIZE*TSDF_BLOCKSIZE*TSDF_BLOCKSIZE, 1.0f);
  encode(empty_data, word_for_empty);
  reduced_dim = word_for_empty.size();
  std::cout<<"Done."<<std::endl;
}

void neural_network::encode(thrust::device_vector<float> &input, thrust::device_vector<float> &output)
{
  // cublasSgemv performs the computation y = alpha*( A*x ) + beta * y
  // since y is our bias vector, we need to make a copy so it doesn't get replaced with the
  // results of the computation. A represents the weights and x is the input to the current layer
  // After each such computation, we also have to evaluate the activation function of the network
  // This is done using thrust::for_each()
  
  for (int L = 0; L < num_layers/2-1; ++L)
  {
    dev_results[L] = dev_biases[L];
  }
  output = dev_biases[num_layers/2-1];
  float alf = 1.0f; float beta = 1.0f;
  
  //cudaStream_t Stream;
  //cudaStreamCreate(&Stream);  

  cublasHandle_t handle;
  (cublasCreate(&handle));

  //cublasSetStream(handle, Stream);
 
  cublasSgemv( 
                handle, 
                CUBLAS_OP_T, 
                network_structure[0], 
                network_structure[1], 
                &alf, 
                (thrust::raw_pointer_cast(dev_weights[0].data())), 
                network_structure[0],
                thrust::raw_pointer_cast(&input[0]), 
                1, 
                &beta,
                thrust::raw_pointer_cast(dev_results[0].data()),
                1
              );
  thrust::for_each(thrust::device,dev_results[0].begin(),dev_results[0].end(),logistic_functor());
  
  for (int i = 1; i < num_layers/2-1; ++i)
  {
    cublasSgemv(  
                  handle, 
                  CUBLAS_OP_T, 
                  network_structure[i], 
                  network_structure[i+1], 
                  &alf, 
                  thrust::raw_pointer_cast(dev_weights[i].data()), 
                  network_structure[i],
                  thrust::raw_pointer_cast(dev_results[i-1].data()), 
                  1, 
                  &beta,
                  thrust::raw_pointer_cast(dev_results[i].data()),
                  1
                );
    thrust::for_each(thrust::device,dev_results[i].begin(),dev_results[i].end(),logistic_functor());
  }

  cublasSgemv( 
    handle, 
    CUBLAS_OP_T, 
    network_structure[num_layers/2-1], 
    network_structure[num_layers/2], 
    &alf, 
    (thrust::raw_pointer_cast(dev_weights[num_layers/2-1].data())), 
    network_structure[num_layers/2-1],
    thrust::raw_pointer_cast(dev_results[num_layers/2-2].data()), 
    1, 
    &beta,
    thrust::raw_pointer_cast(&output[0]),
    1
  );
  thrust::for_each(thrust::device,output.begin(),output.end(),logistic_functor());
 
  (cublasDestroy(handle));
  //cudaStreamDestroy(Stream);  

}

bool neural_network::describes_empty(thrust::device_vector<float> &input, float threshold)
{
  thrust::plus<float> binary_op;
  thrust::device_vector<float> r_vec;
  r_vec.resize(reduced_dim);

  thrust::transform(input.begin(), input.end(), word_for_empty.begin(), r_vec.begin(), sq_diff<float>());
  float distance = thrust::reduce(r_vec.begin(), r_vec.end(), 0.0f, binary_op);

  return (distance < threshold) ? true : false;

}

void neural_network::decode(thrust::device_vector<float> &input, thrust::device_vector<float> &output, activation_fcn which_fcn)
{

  // cublasSgemv performs the computation y = alpha*( A*x ) + beta * y
  // since y is our bias vector, we need to make a copy so it doesn't get replaced with the
  // results of the computation. A represents the weights and x is the input to the current layer
  // After each such computation, we also have to evaluate the activation function of the network
  // This is done using thrust::for_each()


  for (int L = num_layers/2; L < num_layers-1; ++L)
  {
    dev_results[L] = dev_biases[L];
  }
  output = dev_biases[num_layers-1];
  
  float alf = 1.0f; float beta = 1.0f;
  //cudaStream_t Stream;
  //cudaStreamCreate(&Stream);  

  cublasHandle_t handle;
  (cublasCreate(&handle));

  //cublasSetStream(handle, Stream);

  
  cublasSgemv( 
                  handle, 
                  CUBLAS_OP_T, 
                  network_structure[num_layers/2], 
                  network_structure[num_layers/2+1], 
                  &alf, 
                  thrust::raw_pointer_cast(dev_weights[num_layers/2].data()), 
                  network_structure[num_layers/2],
                  thrust::raw_pointer_cast(&input[0]), 
                  1, 
                  &beta,thrust::raw_pointer_cast(dev_results[num_layers/2].data()),
                  1
                );
  if(which_fcn == SIGMOID)
    thrust::for_each(thrust::device,dev_results[num_layers/2].begin(),dev_results[num_layers/2].end(),logistic_functor());
  else if(which_fcn == TANH)
    thrust::for_each(thrust::device,dev_results[num_layers/2].begin(),dev_results[num_layers/2].end(), tanh_functor());


  for (int i = num_layers/2+1; i < num_layers-1; ++i)
  {
    cublasSgemv( 
                  handle, 
                  CUBLAS_OP_T, 
                  network_structure[i], 
                  network_structure[i+1], 
                  &alf, 
                  thrust::raw_pointer_cast(dev_weights[i].data()), 
                  network_structure[i],
                  thrust::raw_pointer_cast(dev_results[i-1].data()), 
                  1, 
                  &beta,thrust::raw_pointer_cast(dev_results[i].data()),
                  1
                );
  if(which_fcn == SIGMOID)
      thrust::for_each(thrust::device,dev_results[i].begin(),dev_results[i].end(),logistic_functor());
  else if(which_fcn == TANH)
      thrust::for_each(thrust::device,dev_results[i].begin(),dev_results[i].end(),tanh_functor());

  }
  cublasSgemv( 
              handle, 
              CUBLAS_OP_T, 
              network_structure[num_layers-1], 
              network_structure[num_layers], 
              &alf, 
              thrust::raw_pointer_cast(dev_weights[num_layers-1].data()), 
              network_structure[num_layers-1],
              thrust::raw_pointer_cast(dev_results[num_layers-2].data()), 
              1, 
              &beta,thrust::raw_pointer_cast(&output[0]),
              1
            );
  if(which_fcn == SIGMOID)
    thrust::for_each(thrust::device,output.begin(),output.end(),logistic_functor());  
  else if(which_fcn == TANH)
    thrust::for_each(thrust::device,output.begin(),output.end(),tanh_functor());  

  (cublasDestroy(handle));
  //cudaStreamDestroy(Stream);  

}

void neural_network::compare(thrust::host_vector<float> &input, thrust::host_vector<float> &output)
{
  thrust::device_vector<float> original = input;
  thrust::device_vector<float> reconstructed = input;
  thrust::device_vector<float> descriptor;
 
  // float time;
  // cudaEvent_t start, stop;

  // ( cudaEventCreate(&start) );
  // ( cudaEventCreate(&stop) );
  // ( cudaEventRecord(start, 0) );

  encode(original,descriptor);

  // ( cudaEventRecord(stop, 0) );
  // ( cudaEventSynchronize(stop) );
  // ( cudaEventElapsedTime(&time, start, stop) );
  //printf("%3.2f ", time);

  decode(descriptor,reconstructed);
  output = reconstructed;
}


void neural_network::compare_gold(thrust::host_vector<float> &input ,thrust::host_vector<float> &output )
{
  const int gold_num_layers = 10;
  int nodes[gold_num_layers+1] =     { 4096,     2048,     1024,     512 ,     256,      128,      256,      512,      1024,     2048,     4096};
  int num_outputs[gold_num_layers] = {           nodes[1], nodes[2], nodes[3], nodes[4], nodes[5], nodes[6], nodes[7], nodes[8], nodes[9], nodes[10] };
  int num_inputs[gold_num_layers] =  { nodes[0], nodes[1], nodes[2], nodes[3], nodes[4], nodes[5], nodes[6], nodes[7], nodes[8], nodes[9]};

  output.resize(nodes[gold_num_layers]);

  //allocating neural network container
  float* outputs[gold_num_layers];
  float* activations[gold_num_layers];

  for (int layer_number = 0; layer_number < gold_num_layers; ++layer_number)
  {
    outputs[layer_number] = new float[num_outputs[layer_number]];
    activations[layer_number] = new float[num_outputs[layer_number]];
  }

  //set activations and output to zero so we can compute the sums
  for(uint layer_number = 0; layer_number < gold_num_layers; layer_number++)
    for (uint fill = 0; fill < num_outputs[layer_number]; ++fill)
      outputs[layer_number][fill] = activations[layer_number][fill] = 0.f;

  for(uint layer_number = 0; layer_number < gold_num_layers; ++layer_number)
  {
    // #pragma omp parallel
    for(uint n_out = 0; n_out < num_outputs[layer_number]; ++n_out)
    {
      //compute activations layerwise
      for(uint n_in = 0; n_in < num_inputs[layer_number]; ++n_in)
      {

        activations[layer_number][n_out] += (layer_number == 0) ? (input[n_in]) /*-TRUNC_NEG)/(TRUNC_PLUS-TRUNC_NEG)*/ * host_weights[layer_number][ nodes[layer_number]*n_out + n_in ] :
                                            outputs[layer_number-1][n_in] * host_weights[layer_number][ nodes[layer_number]*n_out + n_in ];
      }
      outputs[layer_number][n_out] = logistic(activations[layer_number][n_out] + host_biases[layer_number][n_out] );
    }
  }

  for(int d=0; d < TSDF_BLOCKSIZE*TSDF_BLOCKSIZE*TSDF_BLOCKSIZE; ++d)
  {
    output[d] = outputs[gold_num_layers-1][d]/**(TRUNC_PLUS-TRUNC_NEG) + TRUNC_NEG*/;
  }

  for (int del = 0; del < gold_num_layers; ++del)
  {
    if(activations[del]!=NULL)
      delete[] activations[del];
    if(outputs[del]!=NULL)
      delete[] outputs[del];
  }
}
