#include "lu.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

__global__ void lu_kernel1(int n, int k, DATA_TYPE *A) {
    int tid = get_thread_id();
    int num_threads = get_group_size();
    int total_elements = _PB_N - k - 1;
    int elements_per_thread = total_elements / num_threads;
    int remainder = total_elements % num_threads;

    int start_idx, end_idx;
    if (tid < remainder) {
        start_idx = tid * (elements_per_thread + 1) + k + 1;
        end_idx = start_idx + (elements_per_thread + 1);
    } else {
        start_idx = remainder * (elements_per_thread + 1) + (tid - remainder) * elements_per_thread + k + 1;
        end_idx = start_idx + elements_per_thread;
    }

    DATA_TYPE tmp = A[k * N + k];
    for (int j = start_idx; j < end_idx; ++j) {
        A[k * N + j] = A[k * N + j] / tmp;
    }
}

__global__ void lu_kernel2(int n, int k, DATA_TYPE *A) {
    int tid = get_thread_id();
    int num_threads = get_group_size();
    int total_elements = (_PB_N - k - 1);
    int elements_per_thread = total_elements / num_threads;
    int remainder = total_elements % num_threads;

    int start_idx, end_idx;
    if (tid < remainder) {
        start_idx = tid * (elements_per_thread + 1) + k + 1;
        end_idx = start_idx + (elements_per_thread + 1);
    } else {
        start_idx = remainder * (elements_per_thread + 1) + (tid - remainder) * elements_per_thread + k + 1;
        end_idx = start_idx + elements_per_thread;
    }

    for (int j = start_idx; j < end_idx; ++j) {
        DATA_TYPE tmp = A[k * N + j];
        for (int i = k + 1; i < _PB_N; ++i) {
            A[i * N + j] = A[i * N + j] - A[i * N + k] * tmp;
        }
    }
}


// __global__ void lu_kernel2(int n, int k, DATA_TYPE *A) {
//     int tid = get_thread_id();
//     int num_threads = get_group_size();
//     int total_elements = (_PB_N - k - 1) * (_PB_N - k - 1);
//     int elements_per_thread = total_elements / num_threads;
//     int remainder = total_elements % num_threads;

//     int start_idx, end_idx;
//     if (tid < remainder) {
//         start_idx = tid * (elements_per_thread + 1);
//         end_idx = start_idx + (elements_per_thread + 1);
//     } else {
//         start_idx = remainder * (elements_per_thread + 1) + (tid - remainder) * elements_per_thread;
//         end_idx = start_idx + elements_per_thread;
//     }

//     for (int idx = start_idx; idx < end_idx; idx++) {
//         int i = k + 1 + idx / (_PB_N - k - 1);
//         int j = k + 1 + idx % (_PB_N - k - 1);
//         A[i * N + j] = A[i * N + j] - A[i * N + k] * A[k * N + j];
//     }

//     // for (int j = start_idx; j < end_idx; ++j) {
//     //     for (int i = k + 1; i < _PB_N; ++i) {
//     //         A[i * N + j] = A[i * N + j] - A[i * N + k] * A[k * N + j];
//     //     }
//     // }
// }
