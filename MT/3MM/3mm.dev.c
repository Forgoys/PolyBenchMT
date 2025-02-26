#include "3mm.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

__global__ void mm3_kernel1(int ni, int nj, int nk, int nl, int nm, DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *E) {
    // 获取当前线程的ID和线程总数
    int thread_id = get_thread_id();   // 获取当前线程ID
    int group_size = get_group_size(); // 获取线程总数

    // 计算总的任务数 (矩阵 E 的元素个数)
    int total_elements = ni * nj;
    int elements_per_thread = total_elements / group_size;
    int remainder = total_elements % group_size;

    // 计算每个线程负责的任务区间
    int start_idx = thread_id * elements_per_thread + (thread_id < remainder ? thread_id : remainder);
    int end_idx = (thread_id + 1) * elements_per_thread + (thread_id + 1 < remainder ? thread_id + 1 : remainder);

    // 遍历线程所负责的任务
    for (int idx = start_idx; idx < end_idx; ++idx) {
        int i = idx / nj; // 计算行索引 (对应原代码中的 i)
        int j = idx % nj; // 计算列索引 (对应原代码中的 j)

        // 确保索引在矩阵范围内
        if (i < ni && j < nj) {
            E[i * nj + j] = 0;
            DATA_TYPE tmp = E[i * nj + j];
            for (int k = 0; k < nk; ++k) {
                tmp += A[i * nk + k] * B[k * nj + j];
            }
            E[i * nj + j] = tmp;
        }
    }
}

__global__ void mm3_kernel2(int ni, int nj, int nk, int nl, int nm, DATA_TYPE *C, DATA_TYPE *D, DATA_TYPE *F) {
    // 获取当前线程的 ID 和线程总数
    int thread_id = get_thread_id();   // 获取当前线程的 ID
    int group_size = get_group_size(); // 获取线程总数

    // 计算总任务数 (矩阵 F 的元素个数)
    int total_elements = nj * nl; // 矩阵 F 的总元素数
    int elements_per_thread = total_elements / group_size;
    int remainder = total_elements % group_size;

    // 计算当前线程负责的任务区间
    int start_idx = thread_id * elements_per_thread + (thread_id < remainder ? thread_id : remainder);
    int end_idx = (thread_id + 1) * elements_per_thread + (thread_id + 1 < remainder ? thread_id + 1 : remainder);

    // 遍历线程所负责的任务
    for (int idx = start_idx; idx < end_idx; ++idx) {
        int i = idx / nl; // 计算行索引 (对应原代码中的 i)
        int j = idx % nl; // 计算列索引 (对应原代码中的 j)

        // 确保索引在矩阵范围内
        if (i < nj && j < nl) {
            F[i * nl + j] = 0;
            DATA_TYPE tmp = F[i * nl + j];
            for (int k = 0; k < nm; ++k) {
                tmp += C[i * nm + k] * D[k * nl + j];
            }
            F[i * nl + j] = tmp;
        }
    }
}

__global__ void mm3_kernel3(int ni, int nj, int nk, int nl, int nm, DATA_TYPE *E, DATA_TYPE *F, DATA_TYPE *G) {
    // 获取当前线程的 ID 和线程总数
    int thread_id = get_thread_id();   // 获取当前线程的 ID
    int group_size = get_group_size(); // 获取线程总数

    // 计算总任务数 (矩阵 G 的元素个数)
    int total_elements = ni * nl; // 矩阵 G 的总元素数
    int elements_per_thread = total_elements / group_size;
    int remainder = total_elements % group_size;

    // 计算当前线程负责的任务区间
    int start_idx = thread_id * elements_per_thread + (thread_id < remainder ? thread_id : remainder);
    int end_idx = (thread_id + 1) * elements_per_thread + (thread_id + 1 < remainder ? thread_id + 1 : remainder);

    // 遍历线程所负责的任务
    for (int idx = start_idx; idx < end_idx; ++idx) {
        int i = idx / nl; // 计算行索引 (对应原代码中的 i)
        int j = idx % nl; // 计算列索引 (对应原代码中的 j)

        // 确保索引在矩阵范围内
        if (i < ni && j < nl) {
            G[i * nl + j] = 0;
            DATA_TYPE tmp = G[i * nl + j];
            for (int k = 0; k < nj; ++k) {
                tmp += E[i * nj + k] * F[k * nl + j];
            }
            G[i * nl + j] = tmp;
        }
    }
}
