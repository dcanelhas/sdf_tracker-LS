//Vector functions
#include <iostream>

#include <thrust/transform_reduce.h>
#include <thrust/functional.h>
#include <thrust/fill.h>
#include <thrust/inner_product.h>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/for_each.h>
#include <thrust/scan.h>
#include <thrust/execution_policy.h>
#include <thrust/system_error.h>


struct printf_functor
{
  __host__ __device__
  void operator()(float x)
  {
    printf("%f ",x);
  }
};

template <typename T>
struct multiply
{
    __host__ __device__
        T operator()(const T& a, const T& x) const { 
            return a * x;
        }
};

template <typename T>
struct sq_diff
{
    __host__ __device__
    T operator()(const T& p, const T& q) const {
        T tmp = p-q;
        return tmp*tmp;
    }
};

template <typename T>
struct abs_diff
{
    __host__ __device__
    T operator()(const T& p, const T& q) const {
        T tmp = p-q;
        return (tmp > 0) ? tmp : -tmp; 
    }
};

template <typename T>
struct subtract
{
    __host__ __device__
    T operator()(const T& a, const T& b) const {
        return a-b;
    }
};


template <typename T>
struct square
{
    __host__ __device__
        T operator()(const T& x) const { 
            return x * x;
        }
};


void vector_difference(const thrust::device_vector<float> &a, const thrust::device_vector<float> &b, thrust::device_vector<float> &c);

float L2sq_distance(const thrust::device_vector<float> &a, const thrust::device_vector<float> &b);
float L1_distance(thrust::device_vector<float> &a,thrust::device_vector<float> &b);
float em_distance(thrust::device_vector<float> &a,thrust::device_vector<float> &b);
void print_device_vector(thrust::device_vector<float> &d_vec);

void weighted_vector_sum_parallel(  
    const  thrust::device_vector<float> &v1, 
    const  thrust::device_vector<float> &v2, 
    const  float weight,
    size_t numel,  
    thrust::device_vector<float> &combination );

void weighted_vector_sum_sequential(  
    const  thrust::device_vector<float> &v1, 
    const  thrust::device_vector<float> &v2,
    const float weight, 
    size_t numel,  
    thrust::device_vector<float> &combination );

float line_search_combination_parallel(  
    const  thrust::device_vector<float> &v1, 
    const  thrust::device_vector<float> &v2, 
    const  thrust::device_vector<float> &reference,
    size_t numel,  
    thrust::device_vector<float> &combination );

float line_search_combination_sequential(  
    const  thrust::device_vector<float> &v1, 
    const  thrust::device_vector<float> &v2, 
    const  thrust::device_vector<float> &reference,
    size_t numel,  
    thrust::device_vector<float> &combination );

void test_vector_functions(void);