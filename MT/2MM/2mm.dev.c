#include "2mm.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

__global__ void mm2_kernel1(int ni, int nj, int nk, int nl, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE *tmp, DATA_TYPE *A, DATA_TYPE *B) {
    // 获取当前线程的线程 ID和总数
    int thread_id = get_thread_id();
    int group_size = get_group_size();

    // 计算每个线程负责处理的数据范围
    int total_elements = ni * nj;
    int elements_per_thread = total_elements / group_size;
    int remainder = total_elements % group_size;

    // 计算起始和结束索引
    int start_idx = thread_id * elements_per_thread;
    int end_idx = (thread_id + 1) * elements_per_thread;

    // 处理余下的部分
    if (thread_id < remainder) {
        start_idx += thread_id;
        end_idx = start_idx + elements_per_thread + 1;
    } else {
        start_idx += remainder;
        end_idx = start_idx + elements_per_thread;
    }

    for (int idx = start_idx; idx < end_idx; idx++) {
        int i = idx / nj;
        int j = idx % nj;
        tmp[i * nj + j] = 0;
    }

    for (int k = 0; k < nk; k++) {
        for (int idx = start_idx; idx < end_idx; idx++) {
            int i = idx / nj;
            int j = idx % nj;
    
            tmp[i * nj + j] += alpha * A[i * nk + k] * B[k * nj + j];
        }
    }
}

__global__ void mm2_kernel2(int ni, int nj, int nk, int nl, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE *tmp, DATA_TYPE *C, DATA_TYPE *D) {
    // 获取当前线程的线程 ID
    int thread_id = get_thread_id();
    // 获取线程的总数
    int group_size = get_group_size();

    // 计算每个线程负责的任务范围
    int total_elements = ni * nl;                          // 任务总数（D矩阵的元素个数）
    int elements_per_thread = total_elements / group_size; // 每个线程负责的元素个数
    int remainder = total_elements % group_size;           // 余下的元素

    // 每个线程的任务范围
    int start_idx = thread_id * elements_per_thread;
    int end_idx = (thread_id + 1) * elements_per_thread;

    // 分配余下的元素给前面的一些线程
    if (thread_id < remainder) {
        start_idx += thread_id;
        end_idx = start_idx + elements_per_thread + 1;
    } else {
        start_idx += remainder;
        end_idx = start_idx + elements_per_thread;
    }

    for (int idx = start_idx; idx < end_idx; idx++) {
        int i = idx / nl; // 计算 i（行索引）
        int j = idx % nl; // 计算 j（列索引）
        D[i * nl + j] *= beta;
    }

    for (int k = 0; k < nj; k++) {
        for (int idx = start_idx; idx < end_idx; idx++) {
            int i = idx / nl; // 计算 i（行索引）
            int j = idx % nl; // 计算 j（列索引）
            D[i * nl + j] += tmp[i * nj + k] * C[k * nl + j];
        }
    }
}
