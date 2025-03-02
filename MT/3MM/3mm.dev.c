#include "3mm.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

__global__ void mm3_kernel1(int ni, int nj, int nk, int nl, int nm, DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *E) {
    int thread_id = get_thread_id();
    int group_size = get_group_size();

    int total_elements = ni * nj;
    int elements_per_thread = total_elements / group_size;
    int remainder = total_elements % group_size;

    int start_idx = thread_id * elements_per_thread + (thread_id < remainder ? thread_id : remainder);
    int end_idx = (thread_id + 1) * elements_per_thread + (thread_id + 1 < remainder ? thread_id + 1 : remainder);

    for (int idx = start_idx; idx < end_idx; ++idx) {
        int i = idx / nj;
        int j = idx % nj;
        E[i * nj + j] = 0;
    }

    for (int k = 0; k < nk; ++k) {
        for (int idx = start_idx; idx < end_idx; ++idx) {
            int i = idx / nj;
            int j = idx % nj;
            if (i < ni && j < nj) {
                E[i * nj + j] += A[i * nk + k] * B[k * nj + j];
            }
        }
    }
}

__global__ void mm3_kernel2(int ni, int nj, int nk, int nl, int nm, DATA_TYPE *C, DATA_TYPE *D, DATA_TYPE *F) {
    int thread_id = get_thread_id();
    int group_size = get_group_size();

    int total_elements = nj * nl;
    int elements_per_thread = total_elements / group_size;
    int remainder = total_elements % group_size;

    int start_idx = thread_id * elements_per_thread + (thread_id < remainder ? thread_id : remainder);
    int end_idx = (thread_id + 1) * elements_per_thread + (thread_id + 1 < remainder ? thread_id + 1 : remainder);

    for (int idx = start_idx; idx < end_idx; ++idx) {
        int i = idx / nl;
        int j = idx % nl;
        F[i * nl + j] = 0;
    }

    for (int k = 0; k < nm; ++k) {
        for (int idx = start_idx; idx < end_idx; ++idx) {
            int i = idx / nl;
            int j = idx % nl;
            if (i < nj && j < nl) {
                F[i * nl + j] += C[i * nm + k] * D[k * nl + j];
            }
        }
    }
}

__global__ void mm3_kernel3(int ni, int nj, int nk, int nl, int nm, DATA_TYPE *E, DATA_TYPE *F, DATA_TYPE *G) {
    int thread_id = get_thread_id();
    int group_size = get_group_size();

    int total_elements = ni * nl;
    int elements_per_thread = total_elements / group_size;
    int remainder = total_elements % group_size;

    int start_idx = thread_id * elements_per_thread + (thread_id < remainder ? thread_id : remainder);
    int end_idx = (thread_id + 1) * elements_per_thread + (thread_id + 1 < remainder ? thread_id + 1 : remainder);

    for (int idx = start_idx; idx < end_idx; ++idx) {
        int i = idx / nl;
        int j = idx % nl;
        G[i * nl + j] = 0;
    }

    for (int k = 0; k < nj; ++k) {
        for (int idx = start_idx; idx < end_idx; ++idx) {
            int i = idx / nl;
            int j = idx % nl;
            if (i < ni && j < nl) {
                G[i * nl + j] += E[i * nj + k] * F[k * nl + j];
            }
        }
    }
}