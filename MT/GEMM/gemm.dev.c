#include "gemm.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

__global__ void gemm_kernel(int ni, int nj, int nk, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE *a, DATA_TYPE *b, DATA_TYPE *c) {
    int thread_id = get_thread_id();
    int group_size = get_group_size();

    int total_elements = ni * nj;
    int elements_per_thread = total_elements / group_size;
    int remainder = total_elements % group_size;

    int start = thread_id * elements_per_thread + (thread_id < remainder ? thread_id : remainder);
    int end = start + elements_per_thread + (thread_id < remainder ? 1 : 0);

    for (int idx = start; idx < end; ++idx) {
        int i = idx / nj;
        int j = idx % nj;
        c[i * nj + j] *= beta;
    }
    
    for (int k = 0; k < nk; ++k) {
        for (int idx = start; idx < end; ++idx) {
            int i = idx / nj;
            int j = idx % nj;
            c[i * nj + j] += alpha * a[i * nk + k] * b[k * nj + j];
        }
    }
}

/*
A[512 x 512], B[512 x 512]

ID: 0, start: 0, end: 10923
ID: 1, start: 10923, end: 21846
ID: 2, start: 21846, end: 32769
ID: 3, start: 32769, end: 43692
ID: 4, start: 43692, end: 54615
ID: 5, start: 54615, end: 65538
ID: 6, start: 65538, end: 76461
ID: 7, start: 76461, end: 87384
ID: 8, start: 87384, end: 98307
ID: 9, start: 98307, end: 109230
ID: 10, start: 109230, end: 120153
ID: 11, start: 120153, end: 131076
ID: 12, start: 131076, end: 141999
ID: 13, start: 141999, end: 152922
ID: 14, start: 152922, end: 163845
ID: 15, start: 163845, end: 174768
ID: 16, start: 174768, end: 185690
ID: 17, start: 185690, end: 196612
ID: 18, start: 196612, end: 207534
ID: 19, start: 207534, end: 218456
ID: 20, start: 218456, end: 229378
ID: 21, start: 229378, end: 240300
ID: 22, start: 240300, end: 251222
ID: 23, start: 251222, end: 262144
*/