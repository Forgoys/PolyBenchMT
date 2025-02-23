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
#include "2DConvolution.h"
#include "hthread_host.h"

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

#define GPU_DEVICE 0

#define RUN_ON_CPU

void conv2D(int ni, int nj, DATA_TYPE POLYBENCH_2D(A, NI, NJ, ni, nj), DATA_TYPE POLYBENCH_2D(B, NI, NJ, ni, nj)) {
    int i, j;
    DATA_TYPE c11, c12, c13, c21, c22, c23, c31, c32, c33;

    c11 = +0.2;
    c21 = +0.5;
    c31 = -0.8;
    c12 = -0.3;
    c22 = +0.6;
    c32 = -0.9;
    c13 = +0.4;
    c23 = +0.7;
    c33 = +0.10;

    for (i = 1; i < _PB_NI - 1; ++i) // 0
    {
        for (j = 1; j < _PB_NJ - 1; ++j) // 1
        {
            B[i][j] = c11 * A[(i - 1)][(j - 1)] + c12 * A[(i + 0)][(j - 1)] + c13 * A[(i + 1)][(j - 1)] + c21 * A[(i - 1)][(j + 0)] + c22 * A[(i + 0)][(j + 0)] + c23 * A[(i + 1)][(j + 0)] + c31 * A[(i - 1)][(j + 1)] + c32 * A[(i + 0)][(j + 1)] + c33 * A[(i + 1)][(j + 1)];
        }
    }
}

void init(int ni, int nj, DATA_TYPE POLYBENCH_2D(A, NI, NJ, ni, nj)) {
    int i, j;

    for (i = 0; i < ni; ++i) {
        for (j = 0; j < nj; ++j) {
            A[i][j] = (float)rand() / RAND_MAX;
        }
    }
}

void compareResults(int ni, int nj, DATA_TYPE POLYBENCH_2D(B, NI, NJ, ni, nj), DATA_TYPE POLYBENCH_2D(B_outputFromDsp, NI, NJ, ni, nj)) {
    int i, j, fail;
    fail = 0;

    // Compare outputs from CPU and GPU
    for (i = 1; i < (ni - 1); i++) {
        for (j = 1; j < (nj - 1); j++) {
            if (percentDiff(B[i][j], B_outputFromDsp[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }

    // Print results
    double fail_percent = (100.0 * fail) / (double)(ni * nj);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void convolution2DDSP(int ni, int nj, DATA_TYPE POLYBENCH_2D(A, NI, NJ, ni, nj), DATA_TYPE POLYBENCH_2D(B, NI, NJ, ni, nj),
                      DATA_TYPE POLYBENCH_2D(B_outputFromDsp, NI, NJ, ni, nj), double *t_DSP_start, double *t_DSP_end) {
    memcpy(B_outputFromDsp, B, sizeof(DATA_TYPE) * ni * nj);

    // 打开设备并载入dat文件
    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    // 构建传参数组
    uint64_t args[4];                    // 4个参数：2个非指针 + 2个指针
    args[0] = (uint64_t)toRawBits(ni);   // 非指针参数
    args[1] = (uint64_t)toRawBits(nj);   // 非指针参数
    args[2] = (uint64_t)A;               // 指针参数
    args[3] = (uint64_t)B_outputFromDsp; // 指针参数

    // 插入计时开始
    polybench_timer_start(t_DSP_start);

    // 创建线程组并运行
    int groupId = hthread_group_create(clusterId, nthreads, kernel, 2, 2, args);
    // 验证线程组创建是否成功
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

/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static void print_array(int ni, int nj, DATA_TYPE POLYBENCH_2D(B, NI, NJ, ni, nj)) {
    int i, j;

    for (i = 0; i < ni; i++)
        for (j = 0; j < nj; j++) {
            fprintf(stderr, DATA_PRINTF_MODIFIER, B[i][j]);
            if ((i * ni + j) % 20 == 0)
                fprintf(stderr, "\n");
        }
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[]) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    /* Retrieve problem size */
    int ni = NI;
    int nj = NJ;

    /* DSP device init. */
    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        return 0;
    }

    POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_RO);
    POLYBENCH_2D_ARRAY_DECL(B, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_WO);
    POLYBENCH_2D_ARRAY_DECL(B_outputFromDsp, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_WO);

    // initialize the arrays
    init(ni, nj, POLYBENCH_ARRAY(A));

    convolution2DDSP(ni, nj, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(B_outputFromDsp), &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU

    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);

    conv2D(ni, nj, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(ni, nj, POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(B_outputFromDsp));

#else // print output to stderr so no dead code elimination

    print_array(ni, nj, POLYBENCH_ARRAY(B_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "A[%d x %d]", ni, nj);
    polybench_PM_print(kernel, clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(A);
    POLYBENCH_FREE_ARRAY(B);
    POLYBENCH_FREE_ARRAY(B_outputFromDsp);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);

    return 0;
}

#include "../../common/polybench.c"
