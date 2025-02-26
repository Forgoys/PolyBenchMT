#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#define POLYBENCH_TIME 1

#include "../../common/MTUtilFuncts.h"
#include "../../common/polybench.h"
#include "hthread_host.h"
#include "syr2k.h"

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

#define GPU_DEVICE 0

#define RUN_ON_CPU

void init_arrays(int ni, int nj,
                 DATA_TYPE *alpha,
                 DATA_TYPE *beta,
                 DATA_TYPE POLYBENCH_2D(A, NI, NJ, ni, nj),
                 DATA_TYPE POLYBENCH_2D(B, NI, NJ, ni, nj),
                 DATA_TYPE POLYBENCH_2D(C, NI, NI, ni, ni)) {
    int i, j;

    *alpha = 32412;
    *beta = 2123;

    for (i = 0; i < ni; i++) {
        for (j = 0; j < nj; j++) {
            A[i][j] = ((DATA_TYPE)i * j) / ni;
            B[i][j] = ((DATA_TYPE)i * j) / ni;
        }
    }

    for (i = 0; i < ni; i++) {
        for (j = 0; j < ni; j++) {
            C[i][j] = ((DATA_TYPE)i * j) / ni;
        }
    }
}

void syr2kCpu(int ni, int nj,
              DATA_TYPE alpha,
              DATA_TYPE beta,
              DATA_TYPE POLYBENCH_2D(A, NI, NJ, ni, nj),
              DATA_TYPE POLYBENCH_2D(B, NI, NJ, ni, nj),
              DATA_TYPE POLYBENCH_2D(C, NI, NI, ni, ni)) {
    int i, j, k;

    /*    C := alpha*A*B' + alpha*B*A' + beta*C */
    for (i = 0; i < _PB_NI; i++) {
        for (j = 0; j < _PB_NI; j++) {
            C[i][j] *= beta;
        }
    }

    for (i = 0; i < _PB_NI; i++) {
        for (j = 0; j < _PB_NI; j++) {
            for (k = 0; k < _PB_NJ; k++) {
                C[i][j] += alpha * A[i][k] * B[j][k];
                C[i][j] += alpha * B[i][k] * A[j][k];
            }
        }
    }
}

void compareResults(int ni, DATA_TYPE POLYBENCH_2D(C, NI, NI, ni, ni), DATA_TYPE POLYBENCH_2D(C_outputFromDsp, NI, NI, ni, ni)) {
    int i, j, fail;
    fail = 0;

    // Compare C with D
    for (i = 0; i < ni; i++) {
        for (j = 0; j < ni; j++) {
            if (percentDiff(C[i][j], C_outputFromDsp[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }
    // Print results
    double fail_percent = (100.0 * fail) / (double)(ni * ni);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void syr2kDsp(int ni, int nj, DATA_TYPE alpha, DATA_TYPE beta, DATA_TYPE POLYBENCH_2D(A, NI, NJ, ni, nj), DATA_TYPE POLYBENCH_2D(B, NI, NJ, ni, nj),
              DATA_TYPE POLYBENCH_2D(C, NI, NI, ni, ni), DATA_TYPE POLYBENCH_2D(C_outputFromDsp, NI, NI, ni, ni), double *t_DSP_start, double *t_DSP_end) {
    memcpy(C_outputFromDsp, C, sizeof(DATA_TYPE) * ni * ni);
    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    // 构建传参数组
    uint64_t args[7]; // 核函数的参数数量是9

    args[0] = (uint64_t)toRawBits(ni);
    args[1] = (uint64_t)toRawBits(nj);
    args[2] = (uint64_t)toRawBits(alpha);
    args[3] = (uint64_t)toRawBits(beta);
    args[4] = (uint64_t)A;
    args[5] = (uint64_t)B;
    args[6] = (uint64_t)C_outputFromDsp;

    // 插入计时开始
    polybench_timer_start(t_DSP_start);

    // 创建线程组并运行
    int groupId = hthread_group_create(clusterId, nthreads,
                                       kernel, 4, 3, args); // 4个非指针参数，5个指针参数

    if (groupId == -1) {
        M_logError("Failed to create threads with %s", kernel);
        return;
    }

    // 等待线程组执行结束
    hthread_group_wait(groupId);

    // 插入计时结束
    polybench_timer_stop(t_DSP_end);
    hthread_group_destroy(groupId);
}

void print_array(int ni, DATA_TYPE POLYBENCH_2D(C, NI, NI, ni, ni)) {
    int i, j;

    for (i = 0; i < ni; i++)
        for (j = 0; j < ni; j++) {
            fprintf(stderr, DATA_PRINTF_MODIFIER, C[i][j]);
            if ((i * ni + j) % 20 == 0)
                fprintf(stderr, "\n");
        }
    fprintf(stderr, "\n");
}

int main() {
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
    DATA_TYPE alpha;
    DATA_TYPE beta;
    POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_RO);
    POLYBENCH_2D_ARRAY_DECL(B, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_RO);
    POLYBENCH_2D_ARRAY_DECL(C, DATA_TYPE, NI, NI, ni, ni, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(C_outputFromDsp, DATA_TYPE, NI, NI, ni, ni, clusterId, HT_MEM_RW);

    init_arrays(ni, nj, &alpha, &beta, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C));

    syr2kDsp(ni, nj, alpha, beta, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C), POLYBENCH_ARRAY(C_outputFromDsp), &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU

    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);

    syr2kCpu(ni, nj, alpha, beta, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(ni, POLYBENCH_ARRAY(C), POLYBENCH_ARRAY(C_outputFromDsp));

#else // print output to stderr so no dead code elimination

    print_array(ni, POLYBENCH_ARRAY(C_outputFromDsp));

#endif // RUN_ON_CPU
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "A[%d x %d],B[%d x %d]", ni, nj, ni, nj);
    polybench_PM_print(kernel, clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

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
