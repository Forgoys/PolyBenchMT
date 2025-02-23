#include "jacobi1D.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

__global__ void runJacobiDSP_kernel1(int n, DATA_TYPE *A, DATA_TYPE *B) {
    int thread_id = get_thread_id();
    int total_threads = get_group_size();

    // 计算每个线程需要处理的元素数量
    int elements_per_thread = (_PB_N - 2) / total_threads;
    int remainder = (_PB_N - 2) % total_threads;

    // 计算当前线程的起始和结束位置
    int start_idx = 1; // 从索引1开始（跳过边界元素）
    if (thread_id < remainder) {
        // 前remainder个线程多处理一个元素
        start_idx += thread_id * (elements_per_thread + 1);
    } else {
        start_idx += remainder * (elements_per_thread + 1) +
                     (thread_id - remainder) * elements_per_thread;
    }

    // 计算结束索引
    int end_idx;
    if (thread_id < remainder) {
        end_idx = start_idx + elements_per_thread;
    } else {
        end_idx = start_idx + elements_per_thread - 1;
    }

    // 确保不超过数组边界
    end_idx = (end_idx < (_PB_N - 1)) ? end_idx : (_PB_N - 1);

    // 执行计算
    for (int i = start_idx; i <= end_idx; i++) {
        B[i] = 0.33333 * (A[i - 1] + A[i] + A[i + 1]);
    }
    return;
}

__global__ void runJacobiDSP_kernel2(int n, DATA_TYPE *A, DATA_TYPE *B) {
    int thread_id = get_thread_id();
    int total_threads = get_group_size();

    // 计算每个线程需要处理的元素数量
    int elements_per_thread = (_PB_N - 2) / total_threads;
    int remainder = (_PB_N - 2) % total_threads;

    // 计算当前线程的起始和结束位置
    int start_idx = 1; // 从索引1开始（跳过边界元素）
    if (thread_id < remainder) {
        // 前remainder个线程多处理一个元素
        start_idx += thread_id * (elements_per_thread + 1);
    } else {
        start_idx += remainder * (elements_per_thread + 1) +
                     (thread_id - remainder) * elements_per_thread;
    }

    // 计算结束索引
    int end_idx;
    if (thread_id < remainder) {
        end_idx = start_idx + elements_per_thread;
    } else {
        end_idx = start_idx + elements_per_thread - 1;
    }

    // 确保不超过数组边界
    end_idx = (end_idx < (_PB_N - 1)) ? end_idx : (_PB_N - 1);

    // 执行计算
    for (int i = start_idx; i <= end_idx; i++) {
        A[i] = B[i];
    }

    return;
}
