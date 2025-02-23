#include "jacobi2D.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

static inline int min(int a, int b) {
    return a > b ? b : a;
}

__global__ void runJacobi_kernel1(int n, DATA_TYPE* A, DATA_TYPE* B) 
{
    int total_threads = get_group_size();
    int thread_id = get_thread_id();
    
    // 计算每个线程需要处理的行数
    int rows_per_thread = (_PB_N - 2 + total_threads - 1) / total_threads;
    int start_row = 1 + thread_id * rows_per_thread;
    int end_row = min(start_row + rows_per_thread, _PB_N - 1);
    
    // 每个线程处理其负责的行
    for(int i = start_row; i < end_row; i++) {
        for(int j = 1; j < _PB_N-1; j++) {
            B[i*N + j] = 0.2f * (A[i*N + j] + 
                                A[i*N + (j-1)] + 
                                A[i*N + (j+1)] + 
                                A[(i+1)*N + j] + 
                                A[(i-1)*N + j]);
        }
    }
}

__global__ void runJacobi_kernel2(int n, DATA_TYPE* A, DATA_TYPE* B) 
{
    int total_threads = get_group_size();
    int thread_id = get_thread_id();
    
    // 计算每个线程需要处理的行数
    int rows_per_thread = (_PB_N - 2 + total_threads - 1) / total_threads;
    int start_row = 1 + thread_id * rows_per_thread;
    int end_row = min(start_row + rows_per_thread, _PB_N - 1);
    
    // 每个线程处理其负责的行
    for(int i = start_row; i < end_row; i++) {
        for(int j = 1; j < _PB_N-1; j++) {
            A[i*N + j] = B[i*N + j];
        }
    }
}