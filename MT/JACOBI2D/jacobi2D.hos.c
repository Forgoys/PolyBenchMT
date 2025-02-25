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
#include "hthread_host.h"
#include "jacobi2D.h"

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

#define RUN_ON_CPU

void init_array(int n, DATA_TYPE POLYBENCH_2D(A, N, N, n, n), DATA_TYPE POLYBENCH_2D(B, N, N, n, n)) {
    int i, j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            A[i][j] = ((DATA_TYPE)i * (j + 2) + 10) / N;
            B[i][j] = ((DATA_TYPE)(i - 4) * (j - 1) + 11) / N;
        }
    }
}

void runJacobi2DCpu(int tsteps, int n, DATA_TYPE POLYBENCH_2D(A, N, N, n, n), DATA_TYPE POLYBENCH_2D(B, N, N, n, n)) {
    for (int t = 0; t < _PB_TSTEPS; t++) {
        for (int i = 1; i < _PB_N - 1; i++) {
            for (int j = 1; j < _PB_N - 1; j++) {
                B[i][j] = 0.2f * (A[i][j] + A[i][(j - 1)] + A[i][(1 + j)] + A[(1 + i)][j] + A[(i - 1)][j]);
            }
        }

        for (int i = 1; i < _PB_N - 1; i++) {
            for (int j = 1; j < _PB_N - 1; j++) {
                A[i][j] = B[i][j];
            }
        }
    }
}

void compareResults(int n, DATA_TYPE POLYBENCH_2D(a, N, N, n, n), DATA_TYPE POLYBENCH_2D(a_outputFromGpu, N, N, n, n), DATA_TYPE POLYBENCH_2D(b, N, N, n, n), DATA_TYPE POLYBENCH_2D(b_outputFromGpu, N, N, n, n)) {
    int i, j, fail;
    fail = 0;

    // Compare output from CPU and GPU
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (percentDiff(a[i][j], a_outputFromGpu[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (percentDiff(b[i][j], b_outputFromGpu[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }

    // Print results
    double fail_percent = (100.0 * fail) / (double)(2 * n * n);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void runJacobi2DDSP(int tsteps, int n, DATA_TYPE POLYBENCH_2D(A, N, N, n, n), DATA_TYPE POLYBENCH_2D(B, N, N, n, n),
                    DATA_TYPE POLYBENCH_2D(A_outputFromDsp, N, N, n, n), DATA_TYPE POLYBENCH_2D(B_outputFromDsp, N, N, n, n), double *t_DSP_start, double *t_DSP_end) {
    memcpy(A_outputFromDsp, A, sizeof(DATA_TYPE) * N * N);
    memcpy(B_outputFromDsp, B, sizeof(DATA_TYPE) * N * N);

    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    uint64_t args[3];
    args[0] = (uint64_t)toRawBits(n);    // 非指针变量
    args[1] = (uint64_t)A_outputFromDsp; // 指针变量
    args[2] = (uint64_t)B_outputFromDsp; // 指针变量
    int groupId = hthread_group_create(clusterId, nthreads);
    polybench_timer_start(t_DSP_start);
    for (int t = 0; t < _PB_TSTEPS; t++) {
        int rect = hthread_group_exec(groupId, kernel1, 1, 2, args);
        if (rect != HT_SUCCESS) {
            M_logError("Failed to exec threads with %s", kernel1);
            return;
        }
        hthread_group_wait(groupId);
        rect = hthread_group_exec(groupId, kernel2, 1, 2, args);
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
static void print_array(int n, DATA_TYPE POLYBENCH_2D(A, N, N, n, n)) {
    int i, j;

    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++) {
            fprintf(stderr, DATA_PRINTF_MODIFIER, A[i][j]);
            if ((i * n + j) % 20 == 0)
                fprintf(stderr, "\n");
        }
    fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    /* Retrieve problem size. */
    int n = N;
    int tsteps = TSTEPS;

    /* DSP device init. */
    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        return 0;
    }

    POLYBENCH_2D_ARRAY_DECL(a, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(b, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(a_outputFromDsp, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(b_outputFromDsp, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RW);

    init_array(n, POLYBENCH_ARRAY(a), POLYBENCH_ARRAY(b));

    runJacobi2DDSP(tsteps, n, POLYBENCH_ARRAY(a), POLYBENCH_ARRAY(b), POLYBENCH_ARRAY(a_outputFromDsp), POLYBENCH_ARRAY(b_outputFromDsp), &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU

    init_array(n, POLYBENCH_ARRAY(a), POLYBENCH_ARRAY(b));
    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);

    runJacobi2DCpu(tsteps, n, POLYBENCH_ARRAY(a), POLYBENCH_ARRAY(b));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(n, POLYBENCH_ARRAY(a), POLYBENCH_ARRAY(a_outputFromDsp), POLYBENCH_ARRAY(b), POLYBENCH_ARRAY(b_outputFromDsp));

#else // print output to stderr so no dead code elimination

    print_array(n, POLYBENCH_ARRAY(a_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "A[%d], B[%d], tsteps: %d", N, N, TSTEPS);
    polybench_PM_print("jacobi1D", clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(a);
    POLYBENCH_FREE_ARRAY(a_outputFromDsp);
    POLYBENCH_FREE_ARRAY(b);
    POLYBENCH_FREE_ARRAY(b_outputFromDsp);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);
    return 0;
}

#include "../../common/polybench.c"
