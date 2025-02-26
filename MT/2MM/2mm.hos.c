#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define POLYBENCH_TIME 1

#include "../../common/MTUtilFuncts.h"
#include "../../common/polybench.h"
#include "2mm.h"
#include "hthread_host.h"

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

#define RUN_ON_CPU

void init_array(int ni, int nj, int nk, int nl, DATA_TYPE *alpha, DATA_TYPE *beta, DATA_TYPE POLYBENCH_2D(A, NI, NK, ni, nk),
                DATA_TYPE POLYBENCH_2D(B, NK, NJ, nk, nj), DATA_TYPE POLYBENCH_2D(C, NL, NJ, nl, nj),
                DATA_TYPE POLYBENCH_2D(D, NI, NL, ni, nl)) {
    int i, j;

    *alpha = 32412;
    *beta = 2123;

    for (i = 0; i < ni; i++) {
        for (j = 0; j < nk; j++) {
            A[i][j] = ((DATA_TYPE)i * j) / NI;
        }
    }

    for (i = 0; i < nk; i++) {
        for (j = 0; j < nj; j++) {
            B[i][j] = ((DATA_TYPE)i * (j + 1)) / NJ;
        }
    }

    for (i = 0; i < nl; i++) {
        for (j = 0; j < nj; j++) {
            C[i][j] = ((DATA_TYPE)i * (j + 3)) / NL;
        }
    }

    for (i = 0; i < ni; i++) {
        for (j = 0; j < nl; j++) {
            D[i][j] = ((DATA_TYPE)i * (j + 2)) / NK;
        }
    }
}

void compareResults(int ni, int nl, DATA_TYPE POLYBENCH_2D(D, NI, NL, ni, nl), DATA_TYPE POLYBENCH_2D(D_outputFromDSP, NI, NL, ni, nl)) {
    int i, j, fail;
    fail = 0;

    for (i = 0; i < ni; i++) {
        for (j = 0; j < nl; j++) {
            if (percentDiff(D[i][j], D_outputFromDSP[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }

    // print results
    double fail_percent = (100.0 * fail) / (double)(ni * nl);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void mm2(int ni, int nj, int nk, int nl,
         DATA_TYPE alpha,
         DATA_TYPE beta,
         DATA_TYPE POLYBENCH_2D(tmp, NI, NJ, ni, nj),
         DATA_TYPE POLYBENCH_2D(A, NI, NK, ni, nk),
         DATA_TYPE POLYBENCH_2D(B, NK, NJ, nk, nj),
         DATA_TYPE POLYBENCH_2D(C, NL, NJ, nl, nj),
         DATA_TYPE POLYBENCH_2D(D, NI, NL, ni, nl)) {
    int i, j, k;

    /* D := alpha*A*B*C + beta*D */
    for (i = 0; i < _PB_NI; i++) {
        for (j = 0; j < _PB_NJ; j++) {
            tmp[i][j] = 0;
            for (k = 0; k < _PB_NK; ++k) {
                tmp[i][j] += alpha * A[i][k] * B[k][j];
            }
        }
    }

    for (i = 0; i < _PB_NI; i++) {
        for (j = 0; j < _PB_NL; j++) {
            D[i][j] *= beta;
            for (k = 0; k < _PB_NJ; ++k) {
                D[i][j] += tmp[i][k] * C[k][j];
            }
        }
    }
}

/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static void print_array(int ni, int nl,
                        DATA_TYPE POLYBENCH_2D(D, NI, NL, ni, nl)) {
    int i, j;

    for (i = 0; i < ni; i++)
        for (j = 0; j < nl; j++) {
            fprintf(stderr, DATA_PRINTF_MODIFIER, D[i][j]);
            if ((i * ni + j) % 20 == 0)
                fprintf(stderr, "\n");
        }
    fprintf(stderr, "\n");
}

void mm2DSP(int ni, int nj, int nk, int nl, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE POLYBENCH_2D(tmp, NI, NJ, ni, nj),
            DATA_TYPE POLYBENCH_2D(A, NI, NK, ni, nk), DATA_TYPE POLYBENCH_2D(B, NK, NJ, nk, nj), DATA_TYPE POLYBENCH_2D(C, NL, NJ, nl, nj),
            DATA_TYPE POLYBENCH_2D(D, NI, NL, ni, nl), DATA_TYPE POLYBENCH_2D(D_outputFromDSP, NI, NL, ni, nl), double *t_DSP_start, double *t_DSP_end) {
    memcpy(D_outputFromDSP, D, sizeof(DATA_TYPE) * NI * NL);

    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    uint64_t args1[9];
    args1[0] = (uint64_t)toRawBits(ni);
    args1[1] = (uint64_t)toRawBits(nj);
    args1[2] = (uint64_t)toRawBits(nk);
    args1[3] = (uint64_t)toRawBits(nl);
    args1[4] = (uint64_t)toRawBits(alpha);
    args1[5] = (uint64_t)toRawBits(beta);
    args1[6] = (uint64_t)tmp;
    args1[7] = (uint64_t)A;
    args1[8] = (uint64_t)B;

    uint64_t args2[9];
    args2[0] = (uint64_t)toRawBits(ni);
    args2[1] = (uint64_t)toRawBits(nj);
    args2[2] = (uint64_t)toRawBits(nk);
    args2[3] = (uint64_t)toRawBits(nl);
    args2[4] = (uint64_t)toRawBits(alpha);
    args2[5] = (uint64_t)toRawBits(beta);
    args2[6] = (uint64_t)tmp;
    args2[7] = (uint64_t)C;
    args2[8] = (uint64_t)D_outputFromDSP;

    polybench_timer_start(t_DSP_start);

    // 创建线程组
    int groupId = hthread_group_create(clusterId, nthreads);
    // 验证线程组创建结果
    if (groupId == -1) {
        M_logError("Failed to create group with %d threads.", nthreads);
        return;
    }

    polybench_timer_start(t_DSP_start);

    hthread_group_exec(groupId, kernel1, 6, 3, args1);
    hthread_group_wait(groupId);

    hthread_group_exec(groupId, kernel2, 6, 3, args2);
    hthread_group_wait(groupId);

    polybench_timer_stop(t_DSP_end);
    hthread_group_destroy(groupId);
}

int main(int argc, char **argv) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    /* Retrieve problem size. */
    int ni = NI;
    int nj = NJ;
    int nk = NK;
    int nl = NL;

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
    POLYBENCH_2D_ARRAY_DECL(tmp, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, NI, NK, ni, nk, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(B, DATA_TYPE, NK, NJ, nk, nj, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(C, DATA_TYPE, NL, NJ, nl, nj, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(D, DATA_TYPE, NI, NL, ni, nl, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(D_outputFromDSP, DATA_TYPE, NI, NL, ni, nl, clusterId, HT_MEM_RW);

    init_array(ni, nj, nk, nl, &alpha, &beta, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C), POLYBENCH_ARRAY(D));

    mm2DSP(ni, nj, nk, nl, alpha, beta, POLYBENCH_ARRAY(tmp), POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C),
           POLYBENCH_ARRAY(D), POLYBENCH_ARRAY(D_outputFromDSP), &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU

    init_array(ni, nj, nk, nl, &alpha, &beta, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C), POLYBENCH_ARRAY(D));
    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);
    mm2(ni, nj, nk, nl, alpha, beta, POLYBENCH_ARRAY(tmp), POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C), POLYBENCH_ARRAY(D));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(ni, nl, POLYBENCH_ARRAY(D), POLYBENCH_ARRAY(D_outputFromDSP));

#else // print output to stderr so no dead code elimination

    print_array(n, POLYBENCH_ARRAY(a_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "NI:%d, NJ:%d, NK:%d, NL:%d", ni, nj, nk, nl);
    polybench_PM_print("2mm", clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(tmp);
    POLYBENCH_FREE_ARRAY(A);
    POLYBENCH_FREE_ARRAY(B);
    POLYBENCH_FREE_ARRAY(C);
    POLYBENCH_FREE_ARRAY(D);
    POLYBENCH_FREE_ARRAY(D_outputFromDSP);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);
    return 0;
}

#include "../../common/polybench.c"
