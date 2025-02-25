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
#include "hthread_host.h"
#include "lu.h"
#include <string.h>

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

#define RUN_ON_CPU

void lu(int n, DATA_TYPE POLYBENCH_2D(A, N, N, n, n)) {
    for (int k = 0; k < _PB_N; k++) {
        for (int j = k + 1; j < _PB_N; j++) {
            A[k][j] = A[k][j] / A[k][k];
        }

        for (int i = k + 1; i < _PB_N; i++) {
            for (int j = k + 1; j < _PB_N; j++) {
                A[i][j] = A[i][j] - A[i][k] * A[k][j];
            }
        }
    }
}

void init_array(int n, DATA_TYPE POLYBENCH_2D(A, N, N, n, n)) {
    int i, j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            A[i][j] = ((DATA_TYPE)i * j + 1) / N;
        }
    }
}

void compareResults(int n, DATA_TYPE POLYBENCH_2D(A_cpu, N, N, n, n), DATA_TYPE POLYBENCH_2D(A_outputFromDsp, N, N, n, n)) {
    int i, j, fail;
    fail = 0;

    // Compare a and b
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (percentDiff(A_cpu[i][j], A_outputFromDsp[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }

    // 新增fail_percent计算
    double fail_percent = (100.0 * fail) / (double)(n * n);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void luDSP(int n, DATA_TYPE POLYBENCH_2D(A, N, N, n, n), DATA_TYPE POLYBENCH_2D(A_outputFromDsp, N, N, n, n),
           double *t_DSP_start, double *t_DSP_end) {
    memcpy(A_outputFromDsp, A, sizeof(DATA_TYPE) * N * N);

    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    uint64_t args[3];
    args[0] = (uint64_t)toRawBits(n);
    args[2] = (uint64_t)(A_outputFromDsp);

    int groupId = hthread_group_create(clusterId, nthreads);
    hthread_group_wait(groupId); // 等待线程组处于空闲状态
    polybench_timer_start(t_DSP_start);
    for (int k = 0; k < N; k++) {
        args[1] = (uint64_t)toRawBits(k);

        int rect = hthread_group_exec(groupId, kernel1, 2, 1, args); // 执行
        if (rect != HT_SUCCESS) {
            M_logError("Failed to exec threads with %s", kernel1);
            return;
        }
        hthread_group_wait(groupId);

        rect = hthread_group_exec(groupId, kernel2, 2, 1, args); // 执行
        if (rect != HT_SUCCESS) {
            M_logError("Failed to exec threads with %s", kernel2);
            return;
        }
        hthread_group_wait(groupId);
    }
    polybench_timer_stop(t_DSP_end);
}

/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static void print_array(int n,
                        DATA_TYPE POLYBENCH_2D(A, N, N, n, n))

{
    int i, j;

    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++) {
            fprintf(stderr, DATA_PRINTF_MODIFIER, A[i][j]);
            if ((i * n + j) % 20 == 0)
                fprintf(stderr, "\n");
        }
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[]) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    int n = N;

    /* DSP device init. */
    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        return 0;
    }

    /* Variable declaration/allocation. */
    POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(A_outputFromDsp, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RW);

    init_array(n, POLYBENCH_ARRAY(A));

    luDSP(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(A_outputFromDsp), &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU
    init_array(n, POLYBENCH_ARRAY(A));

    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);

    lu(n, POLYBENCH_ARRAY(A));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(A_outputFromDsp));

#else // print output to stderr so no dead code elimination

    print_array(n, POLYBENCH_ARRAY(A_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "A[%d x %d]", n, n);
    polybench_PM_print("lu_kernel", clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    /* free memory space, Unload the dat file, close the cluster */
    POLYBENCH_FREE_ARRAY(A);
    POLYBENCH_FREE_ARRAY(A_outputFromDsp);
    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);

    return 0;
}

#include "../../common/polybench.c"
