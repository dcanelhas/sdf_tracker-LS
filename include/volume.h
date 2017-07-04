#ifndef VOLUME_H
#define VOLUME_H

#include <cuda_runtime.h>

struct Volume
{
    uint3 size;
    float *data;

    Volume() { size = make_uint3(0,0,0); data = NULL; }
    
    __device__ float operator[]( const uint3 & pos ) const {
        return data[pos.x + pos.y * size.x + pos.z * size.x * size.y];
    }

    __device__ float v(const uint3 & pos) const {
        return data[pos.x + pos.y * size.x + pos.z * size.x * size.y];
    }

    __device__ void set(const uint3 & pos, const float d ){
        data[pos.x + pos.y * size.x + pos.z * size.x * size.y] = d;
    }

    __device__ bool is_safe(const uint3 & pos){
      return (pos.x >0) && (pos.x < size.x) &&
		     (pos.y >0) && (pos.y < size.y) &&
	         (pos.z >0) && (pos.z < size.z);
    }

    void init(uint3 s){
        size = s;
        cudaMalloc((void**)&data, size.x * size.y * size.z * sizeof(float));
        cudaMemset(data, 0.0, size.x * size.y * size.z * sizeof(float));
    }

    void init(uint3 s, const float* src){
        size = s;
        cudaMalloc((void**)&data, size.x * size.y * size.z * sizeof(float));
        cudaMemcpy(data, src, size.x * size.y * size.z * sizeof(float), cudaMemcpyHostToDevice);
    }

    void release(){
        cudaFree(data);
        data = NULL;
    }

    void upload(const float* src){
        cudaMemcpy(data, src, size.x * size.y * size.z * sizeof(float), cudaMemcpyHostToDevice);
    }

    void download(float* dst){
        cudaMemcpy(dst, data, size.x * size.y * size.z * sizeof(float), cudaMemcpyDeviceToHost);
    }


};
#endif



