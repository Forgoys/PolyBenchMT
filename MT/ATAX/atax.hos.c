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
#include "atax.h"
#include "hthread_host.h"

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.5

#ifndef M_PI
#define M_PI 3.14159
#endif

#define RUN_ON_CPU

void init_array(int nx, int ny, DATA_TYPE POLYBENCH_1D(x, NX, nx), DATA_TYPE POLYBENCH_2D(A, NX, NY, nx, ny)) {
    int i, j;

    for (i = 0; i < nx; i++) {
        x[i] = i * M_PI;
        for (j = 0; j < ny; j++) {
            A[i][j] = ((DATA_TYPE)i * j) / NX;
        }
    }
}

void compareResults(int ny, DATA_TYPE POLYBENCH_1D(z, NY, ny), DATA_TYPE POLYBENCH_1D(z_outputFromDSP, NY, ny)) {
    int i, fail;
    fail = 0;

    for (i = 0; i < ny; i++) {
        if (percentDiff(z[i], z_outputFromDSP[i]) > PERCENT_DIFF_ERROR_THRESHOLD) {
            fail++;
        }
    }

    // print results
    double fail_percent = (100.0 * fail) / (double)(ny);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void atax(int nx, int ny, DATA_TYPE POLYBENCH_2D(A, NX, NY, nx, ny), DATA_TYPE POLYBENCH_1D(x, NY, ny), DATA_TYPE POLYBENCH_1D(y, NY, ny),
          DATA_TYPE POLYBENCH_1D(tmp, NX, nx)) {
    int i, j;

    for (i = 0; i < _PB_NY; i++) {
        y[i] = 0;
    }

    for (i = 0; i < _PB_NX; i++) {
        tmp[i] = 0;

        for (j = 0; j < _PB_NY; j++) {
            tmp[i] = tmp[i] + A[i][j] * x[j];
        }

        for (j = 0; j < _PB_NY; j++) {
            y[j] = y[j] + A[i][j] * tmp[i];
        }
    }
}

void ataxDSP(int nx, int ny, DATA_TYPE POLYBENCH_2D(A, NX, NY, nx, ny), DATA_TYPE POLYBENCH_1D(x, NX, nx), DATA_TYPE POLYBENCH_1D(y, NY, ny),
             DATA_TYPE POLYBENCH_1D(tmp, NX, nx), DATA_TYPE POLYBENCH_1D(y_outputFromDSP, NY, ny), double *t_DSP_start, double *t_DSP_end) {
    memcpy(y_outputFromDSP, y, sizeof(DATA_TYPE) * NY);

    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    // 非指针变量需要使用toRawBits转换
    uint64_t args1[5];
    args1[0] = (uint64_t)toRawBits(nx);
    args1[1] = (uint64_t)toRawBits(ny);
    args1[2] = (uint64_t)A;
    args1[3] = (uint64_t)x;
    args1[4] = (uint64_t)tmp;

    uint64_t args2[5];
    args2[0] = (uint64_t)toRawBits(nx);
    args2[1] = (uint64_t)toRawBits(ny);
    args2[2] = (uint64_t)A;
    args2[3] = (uint64_t)y_outputFromDSP;
    args2[4] = (uint64_t)tmp;

    polybench_timer_start(t_DSP_start);

    // 创建线程组
    int groupId = hthread_group_create(clusterId, nthreads);
    // 验证线程组创建结果
    if (groupId == -1) {
        M_logError("Failed to create group with %d threads.", nthreads);
        return;
    }

    polybench_timer_start(t_DSP_start);

    hthread_group_exec(groupId, kernel1, 2, 3, args1);
    hthread_group_wait(groupId);
    hthread_group_exec(groupId, kernel2, 2, 3, args2);
    hthread_group_wait(groupId);

    polybench_timer_stop(t_DSP_end);
    hthread_group_destroy(groupId);
}

/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static void print_array(int nx, DATA_TYPE POLYBENCH_1D(y, NX, nx)) {
    int i;

    for (i = 0; i < nx; i++) {
        fprintf(stderr, DATA_PRINTF_MODIFIER, y[i]);
        if (i % 20 == 0)
            fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    /* Retrieve problem size. */
    int nx = NX;
    int ny = NY;

    /* DSP device init. */
    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        return 0;
    }

    POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, NX, NY, nx, ny, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(x, DATA_TYPE, NY, ny, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(y, DATA_TYPE, NY, ny, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(y_outputFromDSP, DATA_TYPE, NY, ny, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(tmp, DATA_TYPE, NX, nx, clusterId, HT_MEM_RW);

    init_array(nx, ny, POLYBENCH_ARRAY(x), POLYBENCH_ARRAY(A));

    ataxDSP(nx, ny, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(x), POLYBENCH_ARRAY(y), POLYBENCH_ARRAY(tmp),
            POLYBENCH_ARRAY(y_outputFromDSP), &t_DSP_start, &t_DSP_end);
#ifdef RUN_ON_CPU

    // init_array(nx, ny, POLYBENCH_ARRAY(x), POLYBENCH_ARRAY(A));

    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);
    atax(nx, ny, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(x), POLYBENCH_ARRAY(y), POLYBENCH_ARRAY(tmp));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(ny, POLYBENCH_ARRAY(y), POLYBENCH_ARRAY(y_outputFromDSP));

#else // print output to stderr so no dead code elimination

    print_array(n, POLYBENCH_ARRAY(a_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "NX:%d, NY:%d", NX, NY);
    polybench_PM_print("atax", clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(A);
    POLYBENCH_FREE_ARRAY(x);
    POLYBENCH_FREE_ARRAY(y);
    POLYBENCH_FREE_ARRAY(y_outputFromDSP);
    POLYBENCH_FREE_ARRAY(tmp);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);
    return 0;
}

#include "../../common/polybench.c"
