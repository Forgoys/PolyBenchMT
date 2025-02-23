#include "2DConvolution.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

static inline int min(int a, int b) {
    return a > b ? b : a;
}

__global__ void convolution2D_kernel(int ni, int nj, DATA_TYPE *A, DATA_TYPE *B)
{
    int group_size = get_group_size();
    int thread_id = get_thread_id();

    DATA_TYPE c11, c12, c13, c21, c22, c23, c31, c32, c33;
    c11 = +0.2;  c21 = +0.5;  c31 = -0.8;
    c12 = -0.3;  c22 = +0.6;  c32 = -0.9;
    c13 = +0.4;  c23 = +0.7;  c33 = +0.10;

    const int total_tasks = (ni-2) * (nj-2);
    if (total_tasks <= 0) return;

    // 任务分配策略：前remainder个线程多处理1个任务
    const int base_tasks = total_tasks / group_size;
    const int remainder = total_tasks % group_size;
    
    int start = (thread_id < remainder) 
                ? thread_id * (base_tasks + 1) 
                : remainder * (base_tasks + 1) + (thread_id - remainder) * base_tasks;
    int end = start + ((thread_id < remainder) ? (base_tasks + 1) : base_tasks);

    for (int t = start; t < end; ++t) {
        // 将一维任务编号转换为二维坐标
        const int i = 1 + t / (nj-2); // i ∈ [1, ni-2]
        const int j = 1 + t % (nj-2); // j ∈ [1, nj-2]

        // 原计算逻辑（边界检查已通过任务分配保证）
        B[i*nj + j] = 
            c11 * A[(i-1)*nj + (j-1)] + c21 * A[(i-1)*nj + j] + c31 * A[(i-1)*nj + (j+1)] +
            c12 * A[i*nj + (j-1)]     + c22 * A[i*nj + j]     + c32 * A[i*nj + (j+1)]     +
            c13 * A[(i+1)*nj + (j-1)] + c23 * A[(i+1)*nj + j] + c33 * A[(i+1)*nj + (j+1)];
    }
}