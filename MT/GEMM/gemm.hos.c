#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define POLYBENCH_TIME 1

#include "../../common/MTUtilFuncts.h"
#include "../../common/polybench.h"
#include "gemm.h"
#include "hthread_host.h"

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

/* Declared constant values for ALPHA and BETA (same as values in PolyBench 2.0) */
#define ALPHA 32412.0f
#define BETA 2123.0f

#define RUN_ON_CPU

void gemm(int ni, int nj, int nk, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE POLYBENCH_2D(A, NI, NK, ni, nk),
          DATA_TYPE POLYBENCH_2D(B, NK, NJ, nk, nj), DATA_TYPE POLYBENCH_2D(C, NI, NJ, ni, nj)) {
    int i, j, k;

    for (i = 0; i < _PB_NI; i++) {
        for (j = 0; j < _PB_NJ; j++) {
            C[i][j] *= beta;

            for (k = 0; k < _PB_NK; ++k) {
                C[i][j] += alpha * A[i][k] * B[k][j];
            }
        }
    }
}

void init(int ni, int nj, int nk, DATA_TYPE *alpha, DATA_TYPE *beta, DATA_TYPE POLYBENCH_2D(A, NI, NK, ni, nk),
          DATA_TYPE POLYBENCH_2D(B, NK, NJ, nk, nj), DATA_TYPE POLYBENCH_2D(C, NI, NJ, ni, nj)) {
    int i, j;

    *alpha = ALPHA;
    *beta = BETA;

    for (i = 0; i < ni; i++) {
        for (j = 0; j < nk; j++) {
            A[i][j] = ((DATA_TYPE)i * j) / NI;
        }
    }

    for (i = 0; i < nk; i++) {
        for (j = 0; j < nj; j++) {
            B[i][j] = ((DATA_TYPE)i * j) / NI;
        }
    }

    for (i = 0; i < ni; i++) {
        for (j = 0; j < nj; j++) {
            C[i][j] = ((DATA_TYPE)i * j) / NI;
        }
    }
}

void compareResults(int ni, int nj, DATA_TYPE POLYBENCH_2D(C, NI, NJ, ni, nj),
                    DATA_TYPE POLYBENCH_2D(C_outputFromDsp, NI, NJ, ni, nj)) {
    int i, j, fail;
    fail = 0;

    // Compare CPU and GPU outputs
    for (i = 0; i < ni; i++) {
        for (j = 0; j < nj; j++) {
            if (percentDiff(C[i][j], C_outputFromDsp[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }

    // Print results
    double fail_percent = (100.0 * fail) / (double)(ni * nj);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void gemmDSP(int ni, int nj, int nk, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE POLYBENCH_2D(A, NI, NK, ni, nk),
             DATA_TYPE POLYBENCH_2D(B, NK, NJ, nk, nj), DATA_TYPE POLYBENCH_2D(C, NI, NJ, ni, nj),
             DATA_TYPE POLYBENCH_2D(C_outputFromDsp, NI, NJ, ni, nj), double *t_DSP_start, double *t_DSP_end) {
    memcpy(C_outputFromDsp, C, sizeof(DATA_TYPE) * ni * nj);
    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    uint64_t args[8];
    args[0] = (uint64_t)toRawBits(ni);
    args[1] = (uint64_t)toRawBits(nj);
    args[2] = (uint64_t)toRawBits(nk);
    args[3] = (uint64_t)toRawBits(alpha);
    args[4] = (uint64_t)toRawBits(beta);
    args[5] = (uint64_t)(A); // 指针变量
    args[6] = (uint64_t)(B); // 指针变量
    // !!!!注意这里使用C_outputFromDsp接收结果，C是CPU的结算结果，用于验证!!!!!
    args[7] = (uint64_t)(C_outputFromDsp);

    /* Start DSP timer. */
    polybench_timer_start(t_DSP_start);
    int groupId = hthread_group_create(clusterId, nthreads, kernel, 5, 3, args);
    if (groupId == -1) {
        M_logError("Failed to create threads with %s", kernel);
        return;
    }
    hthread_group_wait(groupId);
    /* Stop DSP timer. */
    polybench_timer_stop(t_DSP_end);
    hthread_group_destroy(groupId);
}

int main(int argc, char **argv) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    /* Retrieve problem size. */
    int ni = NI;
    int nj = NJ;
    int nk = NK;

    /* DSP device init. */
    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        return 0;
    }

    /* Variable declaration/allocation. */
    DATA_TYPE alpha;
    DATA_TYPE beta;
    POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, NI, NK, ni, nk, clusterId, HT_MEM_RO);
    POLYBENCH_2D_ARRAY_DECL(B, DATA_TYPE, NK, NJ, nk, nj, clusterId, HT_MEM_RO);
    POLYBENCH_2D_ARRAY_DECL(C, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(C_outputFromDsp, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_RW);

    init(ni, nj, nk, &alpha, &beta, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C));

    gemmDSP(ni, nj, nk, alpha, beta, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C),
            POLYBENCH_ARRAY(C_outputFromDsp), &t_DSP_start, &t_DSP_end);

// 注意要在前面define
#ifdef RUN_ON_CPU

    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);

    gemm(ni, nj, nk, alpha, beta, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(ni, nj, POLYBENCH_ARRAY(C), POLYBENCH_ARRAY(C_outputFromDsp));

#else // print output to stderr so no dead code elimination

    print_array(ni, nj, POLYBENCH_ARRAY(C_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "A[%d x %d], B[%d x %d]", ni, nk, nk, nj);
    polybench_PM_print(kernel, clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    /* free memory space, Unload the dat file, close the cluster */
    POLYBENCH_FREE_ARRAY(A);
    POLYBENCH_FREE_ARRAY(B);
    POLYBENCH_FREE_ARRAY(C);
    POLYBENCH_FREE_ARRAY(C_outputFromDsp);
    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);

    return 0;
}

#include "../../common/polybench.c"