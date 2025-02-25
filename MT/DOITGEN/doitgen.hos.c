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
#include "doitgen.h"
#include "hthread_host.h"

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

#define RUN_ON_CPU

void doitgenCPU(int nr, int nq, int np,
                DATA_TYPE POLYBENCH_3D(A, NR, NQ, NP, nr, nq, np),
                DATA_TYPE POLYBENCH_2D(C4, NP, NP, np, np),
                DATA_TYPE POLYBENCH_3D(sum, NR, NQ, NP, nr, nq, np)) {
    int r, q, p, s;

    for (r = 0; r < _PB_NR; r++) {
        for (q = 0; q < _PB_NQ; q++) {
            for (p = 0; p < _PB_NP; p++) {
                sum[r][q][p] = 0;
                for (s = 0; s < _PB_NP; s++)
                    sum[r][q][p] += A[r][q][s] * C4[s][p];
            }
            for (p = 0; p < _PB_NP; p++)
                A[r][q][p] = sum[r][q][p];
        }
    }
}

void init_array(int nr, int nq, int np,
                DATA_TYPE POLYBENCH_3D(A, NR, NQ, NP, nr, nq, np),
                DATA_TYPE POLYBENCH_2D(C4, NP, NP, np, np)) {
    int i, j, k;

    for (i = 0; i < nr; i++)
        for (j = 0; j < nq; j++)
            for (k = 0; k < np; k++)
                A[i][j][k] = ((DATA_TYPE)i * j + k) / np;
    for (i = 0; i < np; i++)
        for (j = 0; j < np; j++)
            C4[i][j] = ((DATA_TYPE)i * j) / np;
}

void compareResults(int nr, int nq, int np,
                    DATA_TYPE POLYBENCH_3D(A, NR, NQ, NP, nr, nq, np),
                    DATA_TYPE POLYBENCH_3D(A_outputFromDsp, NR, NQ, NP, nr, nq, np)) {
    int fail = 0;

    for (int r = 0; r < nr; r++) {
        for (int q = 0; q < nq; q++) {
            for (int p = 0; p < np; p++) {
                if (percentDiff(A[r][q][p], A_outputFromDsp[r][q][p]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                    fail++;
                }
            }
        }
    }
    // Print results
    double fail_percent = (100.0 * fail) / (double)(nr * nq * np);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void doitgenDsp(int nr, int nq, int np,
                DATA_TYPE POLYBENCH_3D(sum, NR, NQ, NP, nr, nq, np),
                DATA_TYPE POLYBENCH_3D(A, NR, NQ, NP, nr, nq, np),
                DATA_TYPE POLYBENCH_2D(C4, NP, NP, np, np),
                DATA_TYPE POLYBENCH_3D(A_outputFromDsp, NR, NQ, NP, nr, nq, np),
                double *t_DSP_start, double *t_DSP_end) {
    memcpy(A_outputFromDsp, A, sizeof(DATA_TYPE) * NR * NQ * NP);

    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    int r = 0;
    // 构建传参数组
    uint64_t args[4];
    args[0] = (uint64_t)toRawBits(r);
    args[1] = (uint64_t)sum;
    args[2] = (uint64_t)A_outputFromDsp;
    args[3] = (uint64_t)C4;

    // 创建线程组并运行
    int groupId = hthread_group_create(clusterId, nthreads);
    polybench_timer_start(t_DSP_start);
    for (r = 0; r < NR; r++) {
        args[0] = (uint64_t)toRawBits(r);
        int rect = hthread_group_exec(groupId, kernel1, 1, 3, args);
        if (rect != HT_SUCCESS) {
            M_logError("Failed to exec threads with %s", kernel1);
            return;
        }
        hthread_group_wait(groupId);
        rect = hthread_group_exec(groupId, kernel2, 1, 3, args);
        if (rect != HT_SUCCESS) {
            M_logError("Failed to exec threads with %s", kernel1);
            return;
        }
        hthread_group_wait(groupId);
    }

    // 插入计时结束
    polybench_timer_stop(t_DSP_end);
    hthread_group_destroy(groupId);
}

int main(int argc, char *argv[]) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    /* Retrieve problem size. */
    int nr = NR;
    int nq = NQ;
    int np = NP;

    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        fprintf(stderr, "DSP initialization failed. Please check hardware and configuration.\n");
        return 0;
    }

    POLYBENCH_3D_ARRAY_DECL(A, DATA_TYPE, NR, NQ, NP, nr, nq, np, clusterId, HT_MEM_RW);
    POLYBENCH_3D_ARRAY_DECL(A_outputFromDsp, DATA_TYPE, NR, NQ, NP, nr, nq, np, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(C4, DATA_TYPE, NP, NP, np, np, clusterId, HT_MEM_RO);
    POLYBENCH_3D_ARRAY_DECL(sum, DATA_TYPE, NR, NQ, NP, nr, nq, np, clusterId, HT_MEM_RW);

    init_array(nr, nq, np, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(C4));
    doitgenDsp(nr, nq, np, POLYBENCH_ARRAY(sum), POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(C4), POLYBENCH_ARRAY(A_outputFromDsp), &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU
    init_array(nr, nq, np, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(C4));

    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);
    doitgenCPU(nr, nq, np, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(C4), POLYBENCH_ARRAY(sum));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(nr, nq, np, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(A_outputFromDsp));

#else // print output to stderr so no dead code elimination

    print_array(n, POLYBENCH_ARRAY(a_outputFromDsp));

#endif // RUN_ON_CPU
    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "A[%d x %d x %d]", nr, nq, np);
    polybench_PM_print("doitgen", clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(A);
    POLYBENCH_FREE_ARRAY(C4);
    POLYBENCH_FREE_ARRAY(sum);
    POLYBENCH_FREE_ARRAY(A_outputFromDsp);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);
    return 0;
}

#include "../../common/polybench.c"
