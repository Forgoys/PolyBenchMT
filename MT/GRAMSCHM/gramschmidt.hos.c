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
#include "gramschmidt.h"
#include "hthread_host.h"
#include <string.h>

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

#define RUN_ON_CPU

void gramschmidt(int ni, int nj, DATA_TYPE POLYBENCH_2D(A, NI, NJ, ni, nj), DATA_TYPE POLYBENCH_2D(R, NJ, NJ, nj, nj), DATA_TYPE POLYBENCH_2D(Q, NI, NJ, ni, nj)) {
    int i, j, k;
    DATA_TYPE nrm;
    for (k = 0; k < _PB_NJ; k++) {
        nrm = 0;
        for (i = 0; i < _PB_NI; i++) {
            nrm += A[i][k] * A[i][k];
        }

        R[k][k] = sqrt(nrm);
        for (i = 0; i < _PB_NI; i++) {
            Q[i][k] = A[i][k] / R[k][k];
        }

        for (j = k + 1; j < _PB_NJ; j++) {
            R[k][j] = 0;
            for (i = 0; i < _PB_NI; i++) {
                R[k][j] += Q[i][k] * A[i][j];
            }
            for (i = 0; i < _PB_NI; i++) {
                A[i][j] = A[i][j] - Q[i][k] * R[k][j];
            }
        }
    }
}

/* Array initialization. */
void init_array(int ni, int nj,
                DATA_TYPE POLYBENCH_2D(A, NI, NJ, ni, nj),
                DATA_TYPE POLYBENCH_2D(R, NJ, NJ, nj, nj),
                DATA_TYPE POLYBENCH_2D(Q, NI, NJ, ni, nj)) {
    int i, j;

    for (i = 0; i < ni; i++) {
        for (j = 0; j < nj; j++) {
            A[i][j] = ((DATA_TYPE)i * j) / ni;
            Q[i][j] = ((DATA_TYPE)i * (j + 1)) / nj;
        }
    }

    for (i = 0; i < nj; i++) {
        for (j = 0; j < nj; j++) {
            R[i][j] = ((DATA_TYPE)i * (j + 2)) / nj;
        }
    }
}

void compareResults(int ni, int nj, DATA_TYPE POLYBENCH_2D(A, NI, NJ, ni, nj), DATA_TYPE POLYBENCH_2D(A_outputFromDsp, NI, NJ, ni, nj)) {
    int i, j, fail;
    fail = 0;

    for (i = 0; i < ni; i++) {
        for (j = 0; j < nj; j++) {
            if (percentDiff(A[i][j], A_outputFromDsp[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }

    // 新增fail_percent计算
    double fail_percent = (100.0 * fail) / (double)(ni * nj);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void gramschmidtDSP(int ni, int nj, DATA_TYPE POLYBENCH_2D(A, NI, NJ, ni, nj),
                    DATA_TYPE POLYBENCH_2D(R, NJ, NJ, nj, nj),
                    DATA_TYPE POLYBENCH_2D(Q, NI, NJ, ni, nj),
                    DATA_TYPE POLYBENCH_2D(A_outputFromDsp, NI, NJ, ni, nj),
                    double *t_DSP_start, double *t_DSP_end) {
    memcpy(A_outputFromDsp, A, sizeof(DATA_TYPE) * NI * NJ);

    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    uint64_t args[6];
    args[0] = (uint64_t)toRawBits(ni);
    args[1] = (uint64_t)toRawBits(nj);
    args[3] = (uint64_t)(A_outputFromDsp);
    args[4] = (uint64_t)(R);
    args[5] = (uint64_t)(Q);

    int groupId = hthread_group_create(clusterId, nthreads);
    hthread_group_wait(groupId); // 等待线程组处于空闲状态
    polybench_timer_start(t_DSP_start);
    for (int k = 0; k < _PB_NJ; k++) {
        args[2] = (uint64_t)toRawBits(k);
        int rect = hthread_group_exec(groupId, kernel1, 3, 3, args); // 执行
        if (rect != HT_SUCCESS) {
            M_logError("Failed to exec threads with %s", kernel1);
            return;
        }
        hthread_group_wait(groupId);
        rect = hthread_group_exec(groupId, kernel2, 3, 3, args); // 执行
        if (rect != HT_SUCCESS) {
            M_logError("Failed to exec threads with %s", kernel2);
            return;
        }
        hthread_group_wait(groupId);
        rect = hthread_group_exec(groupId, kernel3, 3, 3, args); // 执行
        if (rect != HT_SUCCESS) {
            M_logError("Failed to exec threads with %s", kernel3);
            return;
        }
        hthread_group_wait(groupId);
    }
    /* Stop DSP timer. */
    polybench_timer_stop(t_DSP_end);
}

/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static void print_array(int ni, int nj, DATA_TYPE POLYBENCH_2D(A, NI, NJ, ni, nj)) {
    int i, j;

    for (i = 0; i < ni; i++)
        for (j = 0; j < nj; j++) {
            fprintf(stderr, DATA_PRINTF_MODIFIER, A[i][j]);
            if (i % 20 == 0)
                fprintf(stderr, "\n");
        }

    fprintf(stderr, "\n");
}
int main(int argc, char *argv[]) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    /* Retrieve problem size. */
    int ni = NI;
    int nj = NJ;

    /* DSP device init. */
    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        return 0;
    }

    /* Variable declaration/allocation. */
    POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(R, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(Q, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(A_outputFromDsp, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_RW);

    init_array(ni, nj, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(R), POLYBENCH_ARRAY(Q));

    gramschmidtDSP(ni, nj, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(R), POLYBENCH_ARRAY(Q),
                   POLYBENCH_ARRAY(A_outputFromDsp), &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU

    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);

    gramschmidt(ni, nj, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(R), POLYBENCH_ARRAY(Q));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(ni, nj, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(A_outputFromDsp));

#else // print output to stderr so no dead code elimination

    print_array(ni, nj, POLYBENCH_ARRAY(A_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "A[%d x %d], R[%d x %d], Q[%d x %d]", ni, nj, ni, nj, ni, nj);
    polybench_PM_print("gramschmidt_kernel", clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    /* free memory space, Unload the dat file, close the cluster */
    POLYBENCH_FREE_ARRAY(A);
    POLYBENCH_FREE_ARRAY(R);
    POLYBENCH_FREE_ARRAY(Q);
    POLYBENCH_FREE_ARRAY(A_outputFromDsp);
    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);

    return 0;
}

#include "../../common/polybench.c"
