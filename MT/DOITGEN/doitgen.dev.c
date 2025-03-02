#include "doitgen.h"
#include "hthread_device.h"
#include <compiler/m3000.h>
#include <math.h>
#include <time.h>

__global__ void doitgen_kernel1(int r, DATA_TYPE *sum, DATA_TYPE *A, DATA_TYPE *C4) {
    // 获取线程ID和线程总数
    int thread_id = get_thread_id();
    int group_size = get_group_size();

    // 计算每个线程需要处理的元素总数
    int total_elements = NP * NQ;
    int elements_per_thread = total_elements / group_size;
    int extra_elements = total_elements % group_size;

    // 计算当前线程需要处理的元素数量
    int my_elements = elements_per_thread + (thread_id < extra_elements ? 1 : 0);

    // 计算当前线程的起始元素索引
    int start_idx;
    if (thread_id < extra_elements) {
        start_idx = thread_id * (elements_per_thread + 1);
    } else {
        start_idx = extra_elements * (elements_per_thread + 1) + (thread_id - extra_elements) * elements_per_thread;
    }

    // 处理分配给当前线程的元素
    for (int i = 0; i < my_elements; i++) {
        int flat_idx = start_idx + i;
        int p = flat_idx % NP;
        int q = flat_idx / NP;

        sum[r * (NQ * NP) + q * NP + p] = (DATA_TYPE)0.0;
    }
    for (int s = 0; s < NP; s++) {
        for (int i = 0; i < my_elements; i++) {
            int flat_idx = start_idx + i;
            int p = flat_idx % NP;
            int q = flat_idx / NP;
            sum[r * (NQ * NP) + q * NP + p] += A[r * (NQ * NP) + q * NP + s] * C4[s * NP + p];
        }
    }
}

__global__ void doitgen_kernel2(int r, DATA_TYPE *sum, DATA_TYPE *A, DATA_TYPE *C4) {
    // 获取线程ID和线程总数
    int thread_id = get_thread_id();
    int group_size = get_group_size();

    // 计算每个线程需要处理的元素总数
    int total_elements = NP * NQ;
    int elements_per_thread = total_elements / group_size;
    int extra_elements = total_elements % group_size;

    // 计算当前线程需要处理的元素数量
    int my_elements = elements_per_thread + (thread_id < extra_elements ? 1 : 0);

    // 计算当前线程的起始元素索引
    int start_idx;
    if (thread_id < extra_elements) {
        start_idx = thread_id * (elements_per_thread + 1);
    } else {
        start_idx = extra_elements * (elements_per_thread + 1) + (thread_id - extra_elements) * elements_per_thread;
    }

    // 处理分配给当前线程的元素
    for (int i = 0; i < my_elements; i++) {
        int flat_idx = start_idx + i;

        // 将一维索引转换回二维索引 (p, q)
        int p = flat_idx % NP;
        int q = flat_idx / NP;

        if ((p < NP) && (q < NQ)) {
            A[r * (NQ * NP) + q * NP + p] = sum[r * (NQ * NP) + q * NP + p];
        }
    }
}