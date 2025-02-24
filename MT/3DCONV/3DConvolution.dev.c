#include "3DConvolution.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

static inline int min(int a, int b) {
    return a > b ? b : a;
}

__global__ void convolution3D_kernel(int ni, int nj, int nk, int i, DATA_TYPE *A, DATA_TYPE *B) {
    // 获取线程总数和当前线程ID
    int total_threads = get_group_size();
    int thread_id = get_thread_id();

    // 计算每个线程需要处理的j和k的范围
    int total_elements = (_PB_NJ - 1) * (_PB_NK - 1);
    int elements_per_thread = total_elements / total_threads;
    int extra_elements = total_elements % total_threads;

    // 确定当前线程的起始和结束位置
    int start_idx = thread_id * elements_per_thread + min(thread_id, extra_elements);
    int end_idx = start_idx + elements_per_thread + (thread_id < extra_elements ? 1 : 0);

    // 常量系数定义
    DATA_TYPE c11, c12, c13, c21, c22, c23, c31, c32, c33;
    c11 = +2;
    c21 = +5;
    c31 = -8;
    c12 = -3;
    c22 = +6;
    c32 = -9;
    c13 = +4;
    c23 = +7;
    c33 = +10;

    // 遍历当前线程负责的范围
    for (int idx = start_idx; idx < end_idx; idx++) {
        // 从一维索引计算j和k的位置
        int j = idx / (_PB_NK - 1) + 1; // +1确保j > 0
        int k = idx % (_PB_NK - 1) + 1; // +1确保k > 0

        // 原有的边界检查
        if ((i < (_PB_NI - 1)) && (j < (_PB_NJ - 1)) && (k < (_PB_NK - 1)) &&
            (i > 0) && (j > 0) && (k > 0)) {
            B[i * (NK * NJ) + j * NK + k] =
                c11 * A[(i - 1) * (NK * NJ) + (j - 1) * NK + (k - 1)] +
                c21 * A[(i - 1) * (NK * NJ) + (j - 1) * NK + (k - 1)] +
                c31 * A[(i - 1) * (NK * NJ) + (j - 1) * NK + (k - 1)] +
                c11 * A[(i - 1) * (NK * NJ) + (j - 1) * NK + (k + 1)] +
                c21 * A[(i - 1) * (NK * NJ) + (j + 0) * NK + (k + 1)] +
                c31 * A[(i - 1) * (NK * NJ) + (j + 1) * NK + (k + 1)] +
                c12 * A[(i + 0) * (NK * NJ) + (j - 1) * NK + (k + 0)] +
                c22 * A[(i + 0) * (NK * NJ) + (j + 0) * NK + (k + 0)] +
                c32 * A[(i + 0) * (NK * NJ) + (j + 1) * NK + (k + 0)] +
                c13 * A[(i + 1) * (NK * NJ) + (j - 1) * NK + (k - 1)] +
                c23 * A[(i + 1) * (NK * NJ) + (j - 1) * NK + (k - 1)] +
                c33 * A[(i + 1) * (NK * NJ) + (j - 1) * NK + (k - 1)] +
                c13 * A[(i + 1) * (NK * NJ) + (j - 1) * NK + (k + 1)] +
                c23 * A[(i + 1) * (NK * NJ) + (j + 0) * NK + (k + 1)] +
                c33 * A[(i + 1) * (NK * NJ) + (j + 1) * NK + (k + 1)];
        }
    }
}