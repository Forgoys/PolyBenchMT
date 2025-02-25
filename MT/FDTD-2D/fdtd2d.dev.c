#include "fdtd2d.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

__global__ void fdtd_step1_kernel(int nx, int ny, int t, DATA_TYPE *_fict_, DATA_TYPE *ex, DATA_TYPE *ey, DATA_TYPE *hz) {
    int thread_id = get_thread_id();    // 获取当前线程ID
    int num_threads = get_group_size(); // 获取线程数

    int total_elements = nx * ny; // 总数据量

    // 确定当前线程需要处理的任务范围
    int start = thread_id * (total_elements / num_threads);
    int end = (thread_id == num_threads - 1) ? total_elements : (thread_id + 1) * (total_elements / num_threads);

    DATA_TYPE tmp = _fict_[t];
    // 遍历当前线程的工作区域
    for (int idx = start; idx < end; idx++) {
        int i = idx / ny; // 计算行
        int j = idx % ny; // 计算列

        if (i < nx && j < ny) {
            if (i == 0) {
                ey[i * ny + j] = tmp;
            } else {
                ey[i * ny + j] = ey[i * ny + j] - 0.5f * (hz[i * ny + j] - hz[(i - 1) * ny + j]);
            }
        }
    }

    // 如果涉及线程同步，请注意这里可能需要加锁或其他同步机制（具体视目标平台而定）。
}

__global__ void fdtd_step2_kernel(int nx, int ny, int t, DATA_TYPE *ex, DATA_TYPE *ey, DATA_TYPE *hz) {
    int thread_id = get_thread_id();    // 获取当前线程ID
    int num_threads = get_group_size(); // 获取线程数

    int total_elements = nx * ny; // 总数据量

    // 计算每个线程负责的任务范围
    int start = thread_id * (total_elements / num_threads);
    int end = (thread_id == num_threads - 1) ? total_elements : (thread_id + 1) * (total_elements / num_threads);

    // 遍历当前线程需要处理的任务区间
    for (int idx = start; idx < end; idx++) {
        int i = idx / ny; // 计算行索引
        int j = idx % ny; // 计算列索引

        if (i < nx && j < ny && j > 0) { // 只处理合法的j值
            ex[i * ny + j] = ex[i * ny + j] - 0.5f * (hz[i * ny + j] - hz[i * ny + (j - 1)]);
        }
    }

    // 同步注意事项：如果在不同线程中需要共享数据，可能会有数据竞争的问题（如ex或hz的写操作）。在这种情况下，线程同步是必需的。
}

__global__ void fdtd_step3_kernel(int nx, int ny, int t, DATA_TYPE *ex, DATA_TYPE *ey, DATA_TYPE *hz) {
    int thread_id = get_thread_id();    // 获取当前线程ID
    int num_threads = get_group_size(); // 获取线程数

    int total_elements = (nx - 1) * (ny - 1); // 总数据量，确保不越界

    // 计算每个线程需要处理的任务区间
    int start = thread_id * (total_elements / num_threads);
    int end = (thread_id == num_threads - 1) ? total_elements : (thread_id + 1) * (total_elements / num_threads);

    // 遍历当前线程需要处理的任务区间
    for (int idx = start; idx < end; idx++) {
        int i = idx / (ny - 1); // 计算行索引
        int j = idx % (ny - 1); // 计算列索引

        if (i < (nx - 1) && j < (ny - 1)) { // 只处理合法的i和j值
            hz[i * ny + j] = hz[i * ny + j] - 0.7f * (ex[i * ny + (j + 1)] - ex[i * ny + j] + ey[(i + 1) * ny + j] - ey[i * ny + j]);
        }
    }

    // 同步注意事项：如果不同线程访问共享内存区域（如hz数组），可能需要考虑同步机制
}