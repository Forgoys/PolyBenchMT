#include "mvt.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

__global__ void mvt_kernel1(int n, DATA_TYPE *a, DATA_TYPE *x1, DATA_TYPE *y_1) {
    int tid = get_thread_id();
    int num_threads = get_group_size();
    int total_elements = _PB_N;
    int elements_per_thread = total_elements / num_threads;
    int remainder = total_elements % num_threads;

    int start_idx, end_idx;
    if (tid < remainder) {
        start_idx = tid * (elements_per_thread + 1);
        end_idx = start_idx + (elements_per_thread + 1);
    } else {
        start_idx = remainder * (elements_per_thread + 1) + (tid - remainder) * elements_per_thread;
        end_idx = start_idx + elements_per_thread;
    }

    for (int i = start_idx; i < end_idx; ++i) {
        DATA_TYPE tmp = x1[i];
        for (int j = 0; j < _PB_N; j++) {
            tmp += a[i * N + j] * y_1[j];
        }
        x1[i] = tmp;
    }
}

__global__ void mvt_kernel2(int n, DATA_TYPE *a, DATA_TYPE *x2, DATA_TYPE *y_2) {
    int tid = get_thread_id();
    int num_threads = get_group_size();
    int total_elements = _PB_N;
    int elements_per_thread = total_elements / num_threads;
    int remainder = total_elements % num_threads;

    int start_idx, end_idx;
    if (tid < remainder) {
        start_idx = tid * (elements_per_thread + 1);
        end_idx = start_idx + (elements_per_thread + 1);
    } else {
        start_idx = remainder * (elements_per_thread + 1) + (tid - remainder) * elements_per_thread;
        end_idx = start_idx + elements_per_thread;
    }

    for (int j = 0; j < _PB_N; j++) {
        for (int i = start_idx; i < end_idx; ++i) {
            x2[i] += a[j * N + i] * y_2[j];
        }
    }
}