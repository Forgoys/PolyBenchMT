#include "gemver.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>
#include <math.h>

__global__ void gemver_kernel1(int n, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE *a, DATA_TYPE *v1, DATA_TYPE *v2, DATA_TYPE *u1, DATA_TYPE *u2) {
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
        for (int j = 0; j < _PB_N; ++j) {
            a[i * N + j] += u1[i] * v1[j] + u2[i] * v2[j];
        }
    }
}

__global__ void gemver_kernel2(int n, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE *a, DATA_TYPE *x, DATA_TYPE *y, DATA_TYPE *z) {
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
            x[i] += beta * a[j * N + i] * y[j];
        }
    }
    for (int i = start_idx; i < end_idx; ++i) {
        x[i] += z[i];
    }
}

__global__ void gemver_kernel3(int n, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE *a, DATA_TYPE *x, DATA_TYPE *w) {
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
        DATA_TYPE tmp = w[i];
        for (int j = 0; j < _PB_N; j++) {
            tmp += alpha * a[i * N + j] * x[j];
        }
        w[i] = tmp;
    }
}
