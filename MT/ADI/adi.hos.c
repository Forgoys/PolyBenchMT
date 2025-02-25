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
#include "adi.h"
#include "hthread_host.h"

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 2.5

#define GPU_DEVICE 0

#define RUN_ON_CPU

void adi(int tsteps, int n, DATA_TYPE POLYBENCH_2D(A, N, N, n, n), DATA_TYPE POLYBENCH_2D(B, N, N, n, n), DATA_TYPE POLYBENCH_2D(X, N, N, n, n)) {
    for (int t = 0; t < _PB_TSTEPS; t++) {
        // printf("================================\n");
        // printf("step %d:\nbefore:\n", t);
        // printf("B\n");
        // print_array(n, B);
        // printf("X\n");
        // print_array(n, X);
        for (int i1 = 0; i1 < _PB_N; i1++) {
            for (int i2 = 1; i2 < _PB_N; i2++) {
                X[i1][i2] = X[i1][i2] - X[i1][(i2 - 1)] * A[i1][i2] / B[i1][(i2 - 1)];
                B[i1][i2] = B[i1][i2] - A[i1][i2] * A[i1][i2] / B[i1][(i2 - 1)];
            }
        }

        for (int i1 = 0; i1 < _PB_N; i1++) {
            X[i1][(N - 1)] = X[i1][(N - 1)] / B[i1][(N - 1)];
        }

        for (int i1 = 0; i1 < _PB_N; i1++) {
            for (int i2 = 0; i2 < _PB_N - 2; i2++) {
                X[i1][(N - i2 - 2)] = (X[i1][(N - 2 - i2)] - X[i1][(N - 2 - i2 - 1)] * A[i1][(N - i2 - 3)]) / B[i1][(N - 3 - i2)];
            }
        }

        for (int i1 = 1; i1 < _PB_N; i1++) {
            for (int i2 = 0; i2 < _PB_N; i2++) {
                X[i1][i2] = X[i1][i2] - X[(i1 - 1)][i2] * A[i1][i2] / B[(i1 - 1)][i2];
                B[i1][i2] = B[i1][i2] - A[i1][i2] * A[i1][i2] / B[(i1 - 1)][i2];
            }
        }

        for (int i2 = 0; i2 < _PB_N; i2++) {
            X[(N - 1)][i2] = X[(N - 1)][i2] / B[(N - 1)][i2];
        }

        for (int i1 = 0; i1 < _PB_N - 2; i1++) {
            for (int i2 = 0; i2 < _PB_N; i2++) {
                X[(N - 2 - i1)][i2] = (X[(N - 2 - i1)][i2] - X[(N - i1 - 3)][i2] * A[(N - 3 - i1)][i2]) / B[(N - 2 - i1)][i2];
            }
        }
        // printf("\nafter:\n");
        // printf("B\n");
        // print_array(n, B);
        // printf("X\n");
        // print_array(n, X);
        // printf("================================\n");
    }
}

void init_array(int n, DATA_TYPE POLYBENCH_2D(A, N, N, n, n), DATA_TYPE POLYBENCH_2D(B, N, N, n, n), DATA_TYPE POLYBENCH_2D(X, N, N, n, n)) {
    int i, j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            X[i][j] = ((DATA_TYPE)i * (j + 1) + 1) / N;
            A[i][j] = ((DATA_TYPE)(i - 1) * (j + 4) + 2) / N;
            B[i][j] = ((DATA_TYPE)(i + 3) * (j + 7) + 3) / N;
        }
    }
}

void compareResults(int n, DATA_TYPE POLYBENCH_2D(B_cpu, N, N, n, n), DATA_TYPE POLYBENCH_2D(B_fromDsp, N, N, n, n), DATA_TYPE POLYBENCH_2D(X_cpu, N, N, n, n),
                    DATA_TYPE POLYBENCH_2D(X_fromDsp, N, N, n, n)) {
    int i, j, fail;
    fail = 0;

    // Compare b and x output on cpu and gpu
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (percentDiff(B_cpu[i][j], B_fromDsp[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (percentDiff(X_cpu[i][j], X_fromDsp[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }

    // Print results
    double fail_percent = (100.0 * fail) / (double)(2 * n * n);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
void print_array(int n, DATA_TYPE POLYBENCH_2D(X, N, N, n, n)) {
    int i, j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            fprintf(stderr, DATA_PRINTF_MODIFIER, X[i][j]);
            // if ((i * N + j) % 20 == 0)
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

void adiDSP(int tsteps, int n, DATA_TYPE POLYBENCH_2D(A, N, N, n, n), DATA_TYPE POLYBENCH_2D(B, N, N, n, n), DATA_TYPE POLYBENCH_2D(X, N, N, n, n),
            DATA_TYPE POLYBENCH_2D(B_outputFromDsp, N, N, n, n), DATA_TYPE POLYBENCH_2D(X_outputFromDsp, N, N, n, n), double *t_DSP_start, double *t_DSP_end) {

    memcpy(B_outputFromDsp, B, sizeof(DATA_TYPE) * N * N);
    memcpy(X_outputFromDsp, X, sizeof(DATA_TYPE) * N * N);

    // 打开设备并载入dat文件
    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    // 构建传参数组
    // 注意：1个非指针参数(n)和3个指针参数(A,B,X)
    uint64_t args1[4];
    args1[0] = (uint64_t)toRawBits(n);    // 非指针变量，需要toRawBits转换
    args1[1] = (uint64_t)A;               // 指针变量，直接转换为uint64_t
    args1[2] = (uint64_t)B_outputFromDsp; // 指针变量，直接转换为uint64_t
    args1[3] = (uint64_t)X_outputFromDsp; // 指针变量，直接转换为uint64_t

    int i1 = 0;
    uint64_t args2[5];
    args2[0] = (uint64_t)toRawBits(n);    // 非指针变量，需要toRawBits转换
    args2[1] = (uint64_t)toRawBits(i1);   // 非指针变量，需要toRawBits转换
    args2[2] = (uint64_t)A;               // 指针变量，直接转换为uint64_t
    args2[3] = (uint64_t)B_outputFromDsp; // 指针变量，直接转换为uint64_t
    args2[4] = (uint64_t)X_outputFromDsp; // 指针变量，直接转换为uint64_t

    // 创建线程组
    int groupId = hthread_group_create(clusterId, nthreads);
    // 验证线程组创建结果
    if (groupId == -1) {
        M_logError("Failed to create group with %d threads.", nthreads);
        return;
    }

    // 插入计时开始代码
    polybench_timer_start(t_DSP_start);

    for (int t = 0; t < _PB_TSTEPS; t++) {
        hthread_group_exec(groupId, kernel1, 1, 3, args1);
        hthread_group_wait(groupId);
        hthread_group_exec(groupId, kernel2, 1, 3, args1);
        hthread_group_wait(groupId);
        hthread_group_exec(groupId, kernel3, 1, 3, args1);
        hthread_group_wait(groupId);

        for (i1 = 1; i1 < _PB_N; i1++) {
            args2[1] = (uint64_t)toRawBits(i1); // 每次迭代更新i1
            hthread_group_exec(groupId, kernel4, 2, 3, args2);
            hthread_group_wait(groupId);
        }

        hthread_group_exec(groupId, kernel5, 1, 3, args1);
        hthread_group_wait(groupId);

        for (i1 = 0; i1 < _PB_N - 2; i1++) {
            args2[1] = (uint64_t)toRawBits(i1); // 每次迭代更新i1
            hthread_group_exec(groupId, kernel6, 2, 3, args2);
            hthread_group_wait(groupId);
        }
    }

    // 插入计时结束代码
    polybench_timer_stop(t_DSP_end);
}

int main(int argc, char *argv[]) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    /* Retrieve problem size. */
    int tsteps = TSTEPS;
    int n = N;

    /* DSP device init. */
    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        return 0;
    }

    POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RO);
    POLYBENCH_2D_ARRAY_DECL(B, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(B_outputFromDsp, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(X, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(X_outputFromDsp, DATA_TYPE, N, N, n, n, clusterId, HT_MEM_RW);

    init_array(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(X));

    adiDSP(tsteps, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(X), POLYBENCH_ARRAY(B_outputFromDsp),
           POLYBENCH_ARRAY(X_outputFromDsp), &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU
    init_array(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(X));

    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);
    adi(tsteps, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(X));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(n, POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(B_outputFromDsp), POLYBENCH_ARRAY(X), POLYBENCH_ARRAY(X_outputFromDsp));

#else

    print_array(n, POLYBENCH_ARRAY(X_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "2D Grid[%d x %d], Iterations: %d", n, n, tsteps);
    polybench_PM_print("ADI", clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(A);
    POLYBENCH_FREE_ARRAY(B);
    POLYBENCH_FREE_ARRAY(B_outputFromDsp);
    POLYBENCH_FREE_ARRAY(X);
    POLYBENCH_FREE_ARRAY(X_outputFromDsp);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);

    return 0;
}

#include "../../common/polybench.c"
