#include "gramschmidt.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>
#include <math.h>

__global__ void gramschmidt_kernel1(int ni, int nj, int k, DATA_TYPE *a, DATA_TYPE *r, DATA_TYPE *q) {
    int tid = get_thread_id();

    if (tid == 0) {
        DATA_TYPE nrm = 0.0;
        for (int i = 0; i < _PB_NI; i++) {
            nrm += a[i * NJ + k] * a[i * NJ + k];
        }
        r[k * NJ + k] = sqrt(nrm);
    }
}

__global__ void gramschmidt_kernel2(int ni, int nj, int k, DATA_TYPE *a, DATA_TYPE *r, DATA_TYPE *q) {
    int tid = get_thread_id();
    int num_threads = get_group_size();
    int total_elements = _PB_NI;
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
        q[i * NJ + k] = a[i * NJ + k] / r[k * NJ + k];
    }
}

__global__ void gramschmidt_kernel3(int ni, int nj, int k, DATA_TYPE *a, DATA_TYPE *r, DATA_TYPE *q) {
    int tid = get_thread_id();
    int num_threads = get_group_size();
    int total_elements = _PB_NJ - k - 1;
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
        r[k * NJ + j] = 0.0;
    }
    for (int i = 0; i < _PB_NI; i++) {
        for (int j = start_idx; j < end_idx; ++j) {
            r[k * NJ + j] += q[i * NJ + k] * a[i * NJ + j];
        }
    }
    for (int i = 0; i < _PB_NI; i++) {
        for (int j = start_idx; j < end_idx; ++j) {
            a[i * NJ + j] -= q[i * NJ + k] * r[k * NJ + j];
        }
    }
}
