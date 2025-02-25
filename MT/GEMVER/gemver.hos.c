#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define POLYBENCH_TIME 1

#include "../../common/MTUtilFuncts.h"
#include "../../common/polybench.h"
#include "gemver.h"
#include "hthread_host.h"
#include <string.h>

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

#define RUN_ON_CPU

void gemver(int n, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE POLYBENCH_2D(A, N, N, n, n), DATA_TYPE POLYBENCH_1D(u1, N, n), DATA_TYPE POLYBENCH_1D(v1, N, n),
            DATA_TYPE POLYBENCH_1D(u2, N, n), DATA_TYPE POLYBENCH_1D(v2, N, n), DATA_TYPE POLYBENCH_1D(w, N, n), DATA_TYPE POLYBENCH_1D(x, N, n), DATA_TYPE POLYBENCH_1D(y, N, n),
            DATA_TYPE POLYBENCH_1D(z, N, n)) {
    int i, j;

    for (i = 0; i < _PB_N; i++) {
        for (j = 0; j < _PB_N; j++) {
            A[i][j] = A[i][j] + u1[i] * v1[j] + u2[i] * v2[j];
        }
    }

    for (i = 0; i < _PB_N; i++) {
        for (j = 0; j < _PB_N; j++) {
            x[i] = x[i] + beta * A[j][i] * y[j];
        }
    }

    for (i = 0; i < _PB_N; i++) {
        x[i] = x[i] + z[i];
    }


    for (i = 0; i < _PB_N; i++) {
        for (j = 0; j < _PB_N; j++) {
            w[i] = w[i] + alpha * A[i][j] * x[j];
        }
    }

}

void init(int n, DATA_TYPE *alpha,
          DATA_TYPE *beta,
          DATA_TYPE POLYBENCH_2D(A, N, N, n, n),
          DATA_TYPE POLYBENCH_1D(u1, N, n),
          DATA_TYPE POLYBENCH_1D(v1, N, n),
          DATA_TYPE POLYBENCH_1D(u2, N, n),
          DATA_TYPE POLYBENCH_1D(v2, N, n),
          DATA_TYPE POLYBENCH_1D(w, N, n),
          DATA_TYPE POLYBENCH_1D(x, N, n),
          DATA_TYPE POLYBENCH_1D(y, N, n),
          DATA_TYPE POLYBENCH_1D(z, N, n)) {
    int i, j;

    *alpha = 43532;
    *beta = 12313;

    for (i = 0; i < N; i++) {
        u1[i] = i;
        u2[i] = (i + 1) / N / 2.0;
        v1[i] = (i + 1) / N / 4.0;
        v2[i] = (i + 1) / N / 6.0;
        y[i] = (i + 1) / N / 8.0;
        z[i] = (i + 1) / N / 9.0;
        x[i] = 0.0;
        w[i] = 0.0;

        for (j = 0; j < N; j++) {
            A[i][j] = ((DATA_TYPE)i * j) / N;
        }
    }
}

void compareResults(int n, DATA_TYPE POLYBENCH_1D(w1, N, n), DATA_TYPE POLYBENCH_1D(w2, N, n)) {
    int i, fail;
    fail = 0;

    for (i = 0; i < N; i++) {
        if (percentDiff(w1[i], w2[i]) > PERCENT_DIFF_ERROR_THRESHOLD) {
            fail++;
        }
    }

    // 新增fail_percent计算
    double fail_percent = (100.0 * fail) / (double)(n);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void gemverDSP(int n, DATA_TYPE alpha, DATA_TYPE beta,
               DATA_TYPE POLYBENCH_2D(A, N, N, n, n),
               DATA_TYPE POLYBENCH_1D(u1, N, n),
               DATA_TYPE POLYBENCH_1D(v1, N, n),
               DATA_TYPE POLYBENCH_1D(u2, N, n),
               DATA_TYPE POLYBENCH_1D(v2, N, n),
               DATA_TYPE POLYBENCH_1D(w, N, n),
               DATA_TYPE POLYBENCH_1D(w_outputFromDsp, N, n),
               DATA_TYPE POLYBENCH_1D(x, N, n),
               DATA_TYPE POLYBENCH_1D(y, N, n),
               DATA_TYPE POLYBENCH_1D(z, N, n),
               double *t_DSP_start, double *t_DSP_end) {
    memcpy(w_outputFromDsp, w, sizeof(DATA_TYPE) * N);

    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    uint64_t args1[8];
    args1[0] = (uint64_t)toRawBits(n);
    args1[1] = (uint64_t)toRawBits(alpha);
    args1[2] = (uint64_t)toRawBits(beta);
    args1[3] = (uint64_t)(A);
    args1[4] = (uint64_t)(v1);
    args1[5] = (uint64_t)(v2);
    args1[6] = (uint64_t)(u1);
    args1[7] = (uint64_t)(u2);

    uint64_t args2[7];
    args2[0] = (uint64_t)toRawBits(n);
    args2[1] = (uint64_t)toRawBits(alpha);
    args2[2] = (uint64_t)toRawBits(beta);
    args2[3] = (uint64_t)(A);
    args2[4] = (uint64_t)(x);
    args2[5] = (uint64_t)(y);
    args2[6] = (uint64_t)(z);

    uint64_t args3[6];
    args3[0] = (uint64_t)toRawBits(n);
    args3[1] = (uint64_t)toRawBits(alpha);
    args3[2] = (uint64_t)toRawBits(beta);
    args3[3] = (uint64_t)(A);
    args3[4] = (uint64_t)(x);
    args3[5] = (uint64_t)(w_outputFromDsp);

    int groupId = hthread_group_create(clusterId, nthreads);
    hthread_group_wait(groupId); // 等待线程组处于空闲状态
    polybench_timer_start(t_DSP_start);

    int rect = hthread_group_exec(groupId, kernel1, 3, 5, args1); // 执行
    if (rect != HT_SUCCESS) {
        M_logError("Failed to exec threads with %s", kernel1);
        return;
    }
    hthread_group_wait(groupId);
    
    rect = hthread_group_exec(groupId, kernel2, 3, 4, args2); // 执行
    if (rect != HT_SUCCESS) {
        M_logError("Failed to exec threads with %s", kernel2);
        return;
    }
    hthread_group_wait(groupId);
    
    rect = hthread_group_exec(groupId, kernel3, 3, 3, args3); // 执行
    if (rect != HT_SUCCESS) {
        M_logError("Failed to exec threads with %s", kernel3);
        return;
    }
    hthread_group_wait(groupId);

    polybench_timer_stop(t_DSP_end);
    hthread_group_destroy(groupId);
}

/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static void print_array(int n, DATA_TYPE POLYBENCH_1D(w, N, n)) {
    int i;

    for (i = 0; i < n; i++) {
        fprintf(stderr, DATA_PRINTF_MODIFIER, w[i]);
        if (i % 20 == 0)
            fprintf(stderr, "\n");
    }
}

int main(int argc, char *argv[]) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    /* Retrieve problem size. */
    int n = N;

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

    POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(u1, DATA_TYPE, N, n, clusterId, HT_MEM_RO);
    POLYBENCH_1D_ARRAY_DECL(v1, DATA_TYPE, N, n, clusterId, HT_MEM_RO);
    POLYBENCH_1D_ARRAY_DECL(u2, DATA_TYPE, N, n, clusterId, HT_MEM_RO);
    POLYBENCH_1D_ARRAY_DECL(v2, DATA_TYPE, N, n, clusterId, HT_MEM_RO);
    POLYBENCH_1D_ARRAY_DECL(w, DATA_TYPE, N, n, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(w_outputFromDsp, DATA_TYPE, N, n, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(x, DATA_TYPE, N, n, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(y, DATA_TYPE, N, n, clusterId, HT_MEM_RO);
    POLYBENCH_1D_ARRAY_DECL(z, DATA_TYPE, N, n, clusterId, HT_MEM_RO);

    init(n, &alpha, &beta,
         POLYBENCH_ARRAY(A),
         POLYBENCH_ARRAY(u1),
         POLYBENCH_ARRAY(v1),
         POLYBENCH_ARRAY(u2),
         POLYBENCH_ARRAY(v2),
         POLYBENCH_ARRAY(w),
         POLYBENCH_ARRAY(x),
         POLYBENCH_ARRAY(y),
         POLYBENCH_ARRAY(z));

    gemverDSP(n, alpha, beta, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(u1), POLYBENCH_ARRAY(v1), POLYBENCH_ARRAY(u2), POLYBENCH_ARRAY(v2),
              POLYBENCH_ARRAY(w), POLYBENCH_ARRAY(w_outputFromDsp), POLYBENCH_ARRAY(x), POLYBENCH_ARRAY(y), POLYBENCH_ARRAY(z),
              &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU

    init(n, &alpha, &beta,
         POLYBENCH_ARRAY(A),
         POLYBENCH_ARRAY(u1),
         POLYBENCH_ARRAY(v1),
         POLYBENCH_ARRAY(u2),
         POLYBENCH_ARRAY(v2),
         POLYBENCH_ARRAY(w),
         POLYBENCH_ARRAY(x),
         POLYBENCH_ARRAY(y),
         POLYBENCH_ARRAY(z));

    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);

    gemver(n, alpha, beta, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(u1), POLYBENCH_ARRAY(v1), POLYBENCH_ARRAY(u2), POLYBENCH_ARRAY(v2),
           POLYBENCH_ARRAY(w), POLYBENCH_ARRAY(x), POLYBENCH_ARRAY(y), POLYBENCH_ARRAY(z));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(n, POLYBENCH_ARRAY(w), POLYBENCH_ARRAY(w_outputFromDsp));

#else // print output to stderr so no dead code elimination

    print_array(10, POLYBENCH_ARRAY(w_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "A[%d x %d], w[%d], x[%d], y[%d], z[%d], u1[%d], u2[%d], v1[%d], v2[%d]",
            n, n, n, n, n, n, n, n, n, n);
    polybench_PM_print("gemver_kernel", clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(A);
    POLYBENCH_FREE_ARRAY(w);
    POLYBENCH_FREE_ARRAY(w_outputFromDsp);
    POLYBENCH_FREE_ARRAY(x);
    POLYBENCH_FREE_ARRAY(y);
    POLYBENCH_FREE_ARRAY(z);
    POLYBENCH_FREE_ARRAY(u1);
    POLYBENCH_FREE_ARRAY(u2);
    POLYBENCH_FREE_ARRAY(v1);
    POLYBENCH_FREE_ARRAY(v2);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);

    return 0;
}

#include "../../common/polybench.c"
