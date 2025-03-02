#include "atax.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

__global__ void atax_kernel1(int nx, int ny, DATA_TYPE *A, DATA_TYPE *x, DATA_TYPE *tmp) {
    // 获取当前线程的线程 ID
    int thread_id = get_thread_id();
    // 获取线程的总数
    int group_size = get_group_size();

    // 计算每个线程需要处理的任务范围
    int total_elements = nx;                               // 任务总数（每个线程处理一行的计算）
    int elements_per_thread = total_elements / group_size; // 每个线程处理的元素数
    int remainder = total_elements % group_size;           // 余下的元素

    // 每个线程的任务范围
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

    // 处理分配给当前线程的任务范围
    for (int i = start_idx; i < end_idx; i++) {
        tmp[i] = 0;
        DATA_TYPE tmpp = tmp[i];
        for (int j = 0; j < ny; j++) {
            tmpp += A[i * ny + j] * x[j];
        }
        tmp[i] = tmpp;
    }
}

__global__ void atax_kernel2(int nx, int ny, DATA_TYPE *A, DATA_TYPE *y, DATA_TYPE *tmp) {
    // 获取当前线程的线程 ID
    int thread_id = get_thread_id();
    // 获取线程的总数
    int group_size = get_group_size();

    // 计算每个线程需要处理的任务范围
    int total_elements = ny;                               // 任务总数（每个线程处理一列的计算）
    int elements_per_thread = total_elements / group_size; // 每个线程处理的元素数
    int remainder = total_elements % group_size;           // 余下的元素

    // 每个线程的任务范围
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

    for (int j = start_idx; j < end_idx; j++) {
        y[j] = 0;
    }

    for (int i = 0; i < nx; i++) {
        for (int j = start_idx; j < end_idx; j++) {
            y[j] += A[i * ny + j] * tmp[i];
        }
    }
}
