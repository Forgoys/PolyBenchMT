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
#include "mvt.h"
#include <string.h>

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

#define RUN_ON_CPU

void init_array(int n, DATA_TYPE POLYBENCH_2D(A, N, N, n, n), DATA_TYPE POLYBENCH_1D(x1, N, n), DATA_TYPE POLYBENCH_1D(x2, N, n), DATA_TYPE POLYBENCH_1D(y1, N, n), DATA_TYPE POLYBENCH_1D(y2, N, n)) {
    int i, j;

    for (i = 0; i < n; i++) {
        x1[i] = ((DATA_TYPE)i) / N;
        x2[i] = ((DATA_TYPE)i + 1) / N;
        y1[i] = ((DATA_TYPE)i + 3) / N;
        y2[i] = ((DATA_TYPE)i + 4) / N;
        for (j = 0; j < n; j++) {
            A[i][j] = ((DATA_TYPE)i * j) / N;
        }
    }
}

void runMvt(int n, DATA_TYPE POLYBENCH_2D(a, N, N, n, n), DATA_TYPE POLYBENCH_1D(x1, N, n), DATA_TYPE POLYBENCH_1D(x2, N, n), DATA_TYPE POLYBENCH_1D(y1, N, n), DATA_TYPE POLYBENCH_1D(y2, N, n)) {
    int i, j;

    for (i = 0; i < _PB_N; i++) {
        for (j = 0; j < N; j++) {
            x1[i] = x1[i] + a[i][j] * y1[j];
        }
    }

    for (i = 0; i < _PB_N; i++) {
        for (j = 0; j < _PB_N; j++) {
            x2[i] = x2[i] + a[j][i] * y2[j];
        }
    }
}

void compareResults(int n, DATA_TYPE POLYBENCH_1D(x1, N, n), DATA_TYPE POLYBENCH_1D(x1_outputFromDsp, N, n), DATA_TYPE POLYBENCH_1D(x2, N, n), DATA_TYPE POLYBENCH_1D(x2_outputFromDsp, N, n)) {
    int i, fail;
    fail = 0;

    for (i = 0; i < n; i++) {
        if (percentDiff(x1[i], x1_outputFromDsp[i]) > PERCENT_DIFF_ERROR_THRESHOLD) {
            fail++;
        }

        if (percentDiff(x2[i], x2_outputFromDsp[i]) > PERCENT_DIFF_ERROR_THRESHOLD) {
            fail++;
        }
    }

    // 新增fail_percent计算
    double fail_percent = (100.0 * fail) / (double)(n);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void mvtDSP(int n, DATA_TYPE POLYBENCH_2D(a, N, N, n, n), DATA_TYPE POLYBENCH_1D(x1, N, n), DATA_TYPE POLYBENCH_1D(x2, N, n), DATA_TYPE POLYBENCH_1D(y_1, N, n), DATA_TYPE POLYBENCH_1D(y_2, N, n),
            DATA_TYPE POLYBENCH_1D(x1_outputFromDsp, N, n), DATA_TYPE POLYBENCH_1D(x2_outputFromDsp, N, n),
            double *t_DSP_start, double *t_DSP_end) {
    memcpy(x1_outputFromDsp, x1, sizeof(DATA_TYPE) * N);
    memcpy(x2_outputFromDsp, x2, sizeof(DATA_TYPE) * N);

    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    uint64_t args1[4];
    args1[0] = (uint64_t)toRawBits(n);
    args1[1] = (uint64_t)(a);
    args1[2] = (uint64_t)(x1_outputFromDsp);
    args1[3] = (uint64_t)(y_1);

    uint64_t args2[4];
    args2[0] = (uint64_t)toRawBits(n);
    args2[1] = (uint64_t)(a);
    args2[2] = (uint64_t)(x2_outputFromDsp);
    args2[3] = (uint64_t)(y_2);

    int groupId = hthread_group_create(clusterId, nthreads);
    hthread_group_wait(groupId); // 等待线程组处于空闲状态
    polybench_timer_start(t_DSP_start);

    int rect = hthread_group_exec(groupId, kernel1, 1, 3, args1); // 执行
    if (rect != HT_SUCCESS) {
        M_logError("Failed to exec threads with %s", kernel1);
        return;
    }
    hthread_group_wait(groupId);
    rect = hthread_group_exec(groupId, kernel2, 1, 3, args2); // 执行
    if (rect != HT_SUCCESS) {
        M_logError("Failed to exec threads with %s", kernel2);
        return;
    }
    hthread_group_wait(groupId);

    polybench_timer_stop(t_DSP_end);
    hthread_group_destroy(groupId);
}

/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static void print_array(int n,
                        DATA_TYPE POLYBENCH_1D(x1, N, n),
                        DATA_TYPE POLYBENCH_1D(x2, N, n))

{
    int i;

    for (i = 0; i < n; i++) {
        fprintf(stderr, DATA_PRINTF_MODIFIER, x1[i]);
        fprintf(stderr, DATA_PRINTF_MODIFIER, x2[i]);
        if (i % 20 == 0)
            fprintf(stderr, "\n");
    }
}

int main() {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    int n = N;

    /* DSP device init. */
    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        return 0;
    }

    POLYBENCH_2D_ARRAY_DECL(a, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(x1, DATA_TYPE, N, n, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(x2, DATA_TYPE, N, n, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(x1_outputFromDsp, DATA_TYPE, N, n, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(x2_outputFromDsp, DATA_TYPE, N, n, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(y_1, DATA_TYPE, N, n, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(y_2, DATA_TYPE, N, n, clusterId, HT_MEM_RW);

    init_array(n, POLYBENCH_ARRAY(a), POLYBENCH_ARRAY(x1), POLYBENCH_ARRAY(x2), POLYBENCH_ARRAY(y_1), POLYBENCH_ARRAY(y_2));

    mvtDSP(n, POLYBENCH_ARRAY(a), POLYBENCH_ARRAY(x1), POLYBENCH_ARRAY(x2), POLYBENCH_ARRAY(y_1),
           POLYBENCH_ARRAY(y_2), POLYBENCH_ARRAY(x1_outputFromDsp), POLYBENCH_ARRAY(x2_outputFromDsp),
           &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU
    init_array(n, POLYBENCH_ARRAY(a), POLYBENCH_ARRAY(x1), POLYBENCH_ARRAY(x2), POLYBENCH_ARRAY(y_1), POLYBENCH_ARRAY(y_2));
    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);

    // run the algorithm on the CPU
    runMvt(n, POLYBENCH_ARRAY(a), POLYBENCH_ARRAY(x1), POLYBENCH_ARRAY(x2), POLYBENCH_ARRAY(y_1), POLYBENCH_ARRAY(y_2));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(n, POLYBENCH_ARRAY(x1), POLYBENCH_ARRAY(x1_outputFromDsp), POLYBENCH_ARRAY(x2), POLYBENCH_ARRAY(x2_outputFromDsp));

#else // print output to stderr so no dead code elimination

    print_array(n, POLYBENCH_ARRAY(x1_outputFromDsp), POLYBENCH_ARRAY(x2_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "a[%d x %d], x1[%d], x2[%d], y1[%d], y2[%d]",
            n, n, n, n, n, n);
    polybench_PM_print("mvt_kernel", clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(a);
    POLYBENCH_FREE_ARRAY(x1);
    POLYBENCH_FREE_ARRAY(x2);
    POLYBENCH_FREE_ARRAY(x1_outputFromDsp);
    POLYBENCH_FREE_ARRAY(x2_outputFromDsp);
    POLYBENCH_FREE_ARRAY(y_1);
    POLYBENCH_FREE_ARRAY(y_2);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);

    return 0;
}

#include "../../common/polybench.c"
