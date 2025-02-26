#include "syr2k.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

static inline int min(int a, int b) {return a > b ? b :a;}

__global__ void syr2k_kernel(int ni, int nj, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE *a, DATA_TYPE *b, DATA_TYPE *c) {
    // 获取线程的 ID 和线程数
    int thread_id = get_thread_id();    // 获取当前线程 ID
    int num_threads = get_group_size(); // 获取线程总数

    // 每个线程负责一部分的 i 和 j
    int work_per_thread = (ni * ni + num_threads - 1) / num_threads; // 每个线程最多处理 work_per_thread 个元素
    int start_idx = thread_id * work_per_thread;                     // 当前线程开始处理的元素索引
    int end_idx = min(start_idx + work_per_thread, ni * ni);         // 当前线程处理的结束索引

    for (int idx = start_idx; idx < end_idx; ++idx) {
        int i = idx / ni; // 计算 i 行
        int j = idx % ni; // 计算 j 列

        if (i < ni && j < ni) {
            DATA_TYPE tmp = c[i * ni + j];
            tmp *= beta;

            // 计算 c[i][j] 时的循环
            for (int k = 0; k < nj; k++) {
                tmp += alpha * a[i * nj + k] * b[j * nj + k] + alpha * b[i * nj + k] * a[j * nj + k];
            }
            c[i * ni + j] = tmp;
        }
    }
}
