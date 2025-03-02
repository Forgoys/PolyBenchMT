#include "bicg.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>
#include <math.h>

__global__ void bicg_kernel1(int nx, int ny, DATA_TYPE *A, DATA_TYPE *r, DATA_TYPE *s) {
    int thread_id = get_thread_id();
    int num_threads = get_group_size();

    // 计算每个线程需要处理的元素数量
    int items_per_thread = ny / num_threads;
    int remainder = ny % num_threads;

    // 计算当前线程的起始和结束位置
    int start_j = thread_id * items_per_thread + (thread_id < remainder ? thread_id : remainder);
    int end_j = start_j + items_per_thread + (thread_id < remainder ? 1 : 0);

    // 先初始化输出数组
    for (int j = start_j; j < end_j; j++) {
        s[j] = 0.0f;
    }
    
    // 交换循环顺序，提高内存访问效率
    for (int i = 0; i < nx; i++) {
        for (int j = start_j; j < end_j; j++) {
            s[j] += r[i] * A[i * ny + j];
        }
    }
}

__global__ void bicg_kernel2(int nx, int ny, DATA_TYPE *A, DATA_TYPE *p, DATA_TYPE *q) {
    int thread_id = get_thread_id();
    int num_threads = get_group_size();

    // 计算每个线程需要处理的元素数量
    int items_per_thread = _PB_NX / num_threads;
    int remainder = _PB_NX % num_threads;

    // 计算当前线程的起始和结束位置
    int start_i = thread_id * items_per_thread + (thread_id < remainder ? thread_id : remainder);
    int end_i = start_i + items_per_thread + (thread_id < remainder ? 1 : 0);

    // 处理分配给当前线程的所有元素
    for (int i = start_i; i < end_i; i++) {
        q[i] = 0.0f;
        DATA_TYPE tmp = q[i];

        for (int j = 0; j < _PB_NY; j++) {
            tmp += A[i * NY + j] * p[j];
        }
        q[i] = tmp;
    }
}
