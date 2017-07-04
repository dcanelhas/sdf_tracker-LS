//Vector functions
#include <iostream>

#include <thrust/transform_reduce.h>
#include <thrust/functional.h>
#include <thrust/fill.h>
#include <thrust/inner_product.h>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/for_each.h>
#include <thrust/execution_policy.h>
#include <thrust/system_error.h>
#include <vector_functions.h>


float L2sq_distance(const thrust::device_vector<float> &a, const thrust::device_vector<float> &b)
{

  thrust::plus<float> binary_op;
  thrust::device_vector<float> r_vec;
  r_vec.resize(a.size());

  //compute the squared euclidean distance between the a and the empty space description vector
  
  thrust::transform(a.begin(), a.end(), b.begin(), r_vec.begin(), sq_diff<float>());
  return thrust::reduce(r_vec.begin(), r_vec.end(), 0.0f, binary_op);
  
}

void print_device_vector(thrust::device_vector<float> &d_vec)
{
  thrust::for_each(thrust::device, d_vec.begin(), d_vec.end(), printf_functor());
}


float L1_distance(thrust::device_vector<float> &a, thrust::device_vector<float> &b)
{

  thrust::plus<float> binary_op;
  thrust::device_vector<float> r_vec;
  r_vec.resize(a.size());
  
  thrust::transform(a.begin(), a.end(), b.begin(), r_vec.begin(), abs_diff<float>());
  return thrust::reduce(r_vec.begin(), r_vec.end(), 0.0f, binary_op); 
}

float em_distance(thrust::device_vector<float> &a, thrust::device_vector<float> &b)
{
/*
  //normalizing input vectors
  thrust::device_vector<float> a_normalized = a;
  thrust::device_vector<float> b_normalized = b;
  float norm_a = std::sqrt(thrust::inner_product(a.begin(), a.end(),a.begin(),0.0f));
  float norm_b = std::sqrt(thrust::inner_product(b.begin(), b.end(),b.begin(),0.0f));
  thrust::transform(a_normalized.begin(), a_normalized.end(), a_normalized.begin(), thrust::placeholders::_1 /= norm_a);
  thrust::transform(b_normalized.begin(), b_normalized.end(), b_normalized.begin(), thrust::placeholders::_1 /= norm_b);
*/
  //computing the cdf's
  thrust::device_vector<float> a_cdf = a/*_normalized*/;
  thrust::device_vector<float> b_cdf = b/*_normalized*/;
  thrust::inclusive_scan(a_cdf.begin(), a_cdf.end(),a_cdf.begin());
  thrust::inclusive_scan(b_cdf.begin(), b_cdf.end(),b_cdf.begin());

  return L1_distance(a_cdf,b_cdf);

}

float line_search_combination_parallel(
const  thrust::device_vector<float> &v1, 
const  thrust::device_vector<float> &v2, 
const  thrust::device_vector<float> &reference,
size_t numel,  
  thrust::device_vector<float> &combination )
{
  thrust::device_vector<float> w1;               w1.resize(numel);
  thrust::device_vector<float> w2;               w2.resize(numel);
  thrust::device_vector<float> weighted_v1;      weighted_v1.resize(numel);
  thrust::device_vector<float> weighted_v2;      weighted_v2.resize(numel);
  combination.resize(numel);

  float best_error = std::numeric_limits<float>::infinity();
  float best_weight = 0.5;
  for (int w_i = 0; w_i < 101; ++w_i)
  {
    float w_f = float(w_i)/100.f;
    thrust::fill(w1.begin(), w1.end(), w_f);
    thrust::fill(w2.begin(), w2.end(), 1.0f - w_f);

    thrust::transform(w1.begin(), w1.end(), v1.begin(), weighted_v1.begin(),
      thrust::multiplies<float>());
    thrust::transform(w2.begin(), w2.end(), v2.begin(), weighted_v2.begin(),
      thrust::multiplies<float>());
    thrust::transform(weighted_v1.begin(), weighted_v1.end(), weighted_v2.begin(), combination.begin(),
      thrust::plus<float>());
   
    float current_err = L2sq_distance(combination, reference);
    if (current_err < best_error)
    {
      best_error = current_err;
      best_weight = w_f;
    }
  }
  
  thrust::fill(w1.begin(), w1.end(), best_weight);
  thrust::fill(w2.begin(), w2.end(), 1.0f - best_weight);

  thrust::transform(w1.begin(), w1.end(), v1.begin(), weighted_v1.begin(),
    thrust::multiplies<float>());
  thrust::transform(w2.begin(), w2.end(), v2.begin(), weighted_v2.begin(),
    thrust::multiplies<float>());
  thrust::transform(weighted_v1.begin(), weighted_v1.end(), weighted_v2.begin(), combination.begin(),
    thrust::plus<float>());

  return best_weight;
}


float line_search_combination_sequential(
const  thrust::device_vector<float> &v1, 
const  thrust::device_vector<float> &v2, 
const  thrust::device_vector<float> &reference,
size_t numel,  
  thrust::device_vector<float> &combination )
{
  thrust::device_vector<float> w1;               w1.resize(numel);
  thrust::device_vector<float> weighted_v2;      weighted_v2.resize(numel);
  combination.resize(numel);

  float best_error = std::numeric_limits<float>::infinity();
  float best_weight = 0.5;
  for (int w_i = 0; w_i < 101; ++w_i)
  {
    float w_f = float(w_i)/100.f;
    thrust::fill(w1.begin(), w1.end(), w_f);

    thrust::transform(w1.begin(), w1.end(), v2.begin(), weighted_v2.begin(),
      thrust::multiplies<float>());
    thrust::transform(v1.begin(), v1.end(), weighted_v2.begin(), combination.begin(),
      thrust::plus<float>());
   
    float current_err = L2sq_distance(combination, reference);
    if (current_err < best_error)
    {
      best_error = current_err;
      best_weight = w_f;
    }
  }
  
  thrust::fill(w1.begin(), w1.end(), best_weight);

  thrust::transform(w1.begin(), w1.end(), v2.begin(), weighted_v2.begin(),
    thrust::multiplies<float>());
  thrust::transform(v1.begin(), v1.end(), weighted_v2.begin(), combination.begin(),
    thrust::plus<float>());

  return best_weight;
}

void weighted_vector_sum_parallel(  
    const  thrust::device_vector<float> &v1, 
    const  thrust::device_vector<float> &v2, 
    const  float weight,
    size_t numel,  
    thrust::device_vector<float> &combination )
{
  thrust::device_vector<float> w1;               w1.resize(numel);
  thrust::device_vector<float> w2;               w2.resize(numel);
  thrust::device_vector<float> weighted_v1;      weighted_v1.resize(numel);
  thrust::device_vector<float> weighted_v2;      weighted_v2.resize(numel);
  combination.resize(numel);

  thrust::fill(w1.begin(), w1.end(), weight);
  thrust::fill(w2.begin(), w2.end(), 1.0f - weight);

  thrust::transform(w1.begin(), w1.end(), v1.begin(), weighted_v1.begin(),
    thrust::multiplies<float>());
  thrust::transform(w2.begin(), w2.end(), v2.begin(), weighted_v2.begin(),
    thrust::multiplies<float>());
  thrust::transform(weighted_v1.begin(), weighted_v1.end(), weighted_v2.begin(), combination.begin(),
    thrust::plus<float>());
}

void weighted_vector_sum_sequential(  
    const  thrust::device_vector<float> &v1, 
    const  thrust::device_vector<float> &v2,
    const float weight, 
    size_t numel,  
    thrust::device_vector<float> &combination )
{
  thrust::device_vector<float> w;               w.resize(numel);
  thrust::device_vector<float> weighted_v2;      weighted_v2.resize(numel);
  combination.resize(numel);
  
  thrust::fill(w.begin(), w.end(), weight);

  thrust::transform(w.begin(), w.end(), v2.begin(), weighted_v2.begin(),
    thrust::multiplies<float>());
  thrust::transform(v1.begin(), v1.end(), weighted_v2.begin(), combination.begin(),
    thrust::plus<float>());
}


void vector_difference(const thrust::device_vector<float> &a, const thrust::device_vector<float> &b, thrust::device_vector<float> &c)
{
  c.resize(a.size());
  thrust::transform(a.begin(), a.end(), b.begin(), c.begin(),
      thrust::minus<float>());
}

void test_vector_functions(void)
{
  thrust::device_vector<float> A;
  thrust::device_vector<float> B;
  thrust::device_vector<float> R;

  A.push_back(0.2f);
  A.push_back(0.4f);
  A.push_back(0.6f);
  A.push_back(0.8f);
  A.push_back(1.0f);

  B.push_back(-0.1f);
  B.push_back(0.2f);
  B.push_back(-0.3f);
  B.push_back(0.4f);
  B.push_back(-0.5f);

  R.push_back(0.3f);
  R.push_back(0.4f);
  R.push_back(0.5f);
  R.push_back(0.6f);
  R.push_back(0.7f);



  std::cout << "testing vector functions..." << std::endl;
  std::cout << "A : " << std::endl;
  print_device_vector(A);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "B : " << std::endl;
  print_device_vector(B);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "R : " << std::endl;
  print_device_vector(R);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "L2sq_distance(A,A): "<< L2sq_distance(A,A) << std::endl;
  std::cout << "L1_distance(A,A): "<< L1_distance(A,A) << std::endl;
  cudaDeviceSynchronize();
  std::cout << "L2sq_distance(A,B): "<< L2sq_distance(A,B) << std::endl;
  std::cout << "L1_distance(A,B): "<< L1_distance(A,B) << std::endl;
  cudaDeviceSynchronize();
  std::cout << "L2sq_distance(B,A): "<< L2sq_distance(B,A) << std::endl;
  std::cout << "L1_distance(B,A): "<< L1_distance(B,A) << std::endl;
  cudaDeviceSynchronize();
  std::cout << "L2sq_distance(B,B): "<< L2sq_distance(B,B) << std::endl;
  std::cout << "L1_distance(B,B): "<< L1_distance(B,B) << std::endl;
  cudaDeviceSynchronize();
  std::cout << "L2sq_distance(A,R): "<< L2sq_distance(A,R) << std::endl;
  std::cout << "L1_distance(A,R): "<< L1_distance(A,R) << std::endl;
  cudaDeviceSynchronize();
  std::cout << "L2sq_distance(B,R): "<< L2sq_distance(B,R) << std::endl;
  std::cout << "L1_distance(B,R): "<< L1_distance(B,R) << std::endl;
  cudaDeviceSynchronize();
  std::cout << "L2sq_distance(R,R): "<< L2sq_distance(R,R) << std::endl;
  std::cout << "L1_distance(R,R): "<< L1_distance(R,R) << std::endl;
  cudaDeviceSynchronize();
  std::cout << "L2sq_distance(R,A): "<< L2sq_distance(R,A) << std::endl;
  std::cout << "L1_distance(R,A): "<< L1_distance(R,A) << std::endl;
  cudaDeviceSynchronize();
  std::cout << "L2sq_distance(R,B): "<< L2sq_distance(R,B) << std::endl;
  std::cout << "L1_distance(R,B): "<< L1_distance(R,B) << std::endl;
  cudaDeviceSynchronize();
  std::cout << "L2sq_distance(R,R): "<< L2sq_distance(R,R) << std::endl;
  std::cout << "L1_distance(R,R): "<< L1_distance(R,R) << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "vector_difference(A,B,C)" << std::endl;
  thrust::device_vector<float> C;
  vector_difference(A,B,C);
  std::cout << "A : " << std::endl;
  print_device_vector(A);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "B : " << std::endl;
  print_device_vector(B);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "C : " << std::endl;
  print_device_vector(C);
  std::cout << std::endl;
  cudaDeviceSynchronize();


  std::cout << "weighted_vector_sum_sequential(A, B, 0.25, 5, C)" << std::endl;
  weighted_vector_sum_sequential(A, B, 0.25, 5, C);
  std::cout << "A : " << std::endl;
  print_device_vector(A);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "B : " << std::endl;
  print_device_vector(B);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "C : " << std::endl;
  print_device_vector(C);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "weighted_vector_sum_parallel(A, B, 0.25, 5, C)" << std::endl;
  weighted_vector_sum_parallel(A, B, 0.25, 5, C);
  std::cout << "A : " << std::endl;
  print_device_vector(A);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "B : " << std::endl;
  print_device_vector(B);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "C : " << std::endl;
  print_device_vector(C);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  float weight = 0;

  std::cout << "line_search_combination_sequential(A, B, R, 5, C)" << std::endl;
  weight = line_search_combination_sequential(A, B, R, 5, C);
  
  std::cout << "w : " << weight << std::endl;

  std::cout << "A : " << std::endl;
  print_device_vector(A);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "B : " << std::endl;
  print_device_vector(B);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "C : " << std::endl;
  print_device_vector(C);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  weight = 0;
  std::cout << "line_search_combination_parallel(A, B, R, 5, C)" << std::endl;
  weight = line_search_combination_parallel(A, B, R, 5, C);
  
  std::cout << "w : " << weight << std::endl;

  std::cout << "A : " << std::endl;
  print_device_vector(A);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "B : " << std::endl;
  print_device_vector(B);
  std::cout << std::endl;
  cudaDeviceSynchronize();
  
  std::cout << "C : " << std::endl;
  print_device_vector(C);
  std::cout << std::endl;
  cudaDeviceSynchronize();


}