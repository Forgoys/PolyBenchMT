#include "syrk.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

__global__ void syrk_kernel(int ni, int nj, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE *a, DATA_TYPE *c) {
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
        for (int j = 0; j < _PB_NI; ++j) {
            DATA_TYPE tmp = c[i * NI + j] * beta;
            int k;
            for (k = 0; k < _PB_NJ; k++) {
                tmp += alpha * a[i * NJ + k] * a[j * NJ + k];
            }
            c[i * NI + j] = tmp;
        }
    }
}