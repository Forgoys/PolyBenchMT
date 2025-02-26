
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define POLYBENCH_TIME 1

#include "../../common/MTUtilFuncts.h"
#include "../../common/polybench.h"
#include "bicg.h"
#include "hthread_host.h"

// Error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.5

#define GPU_DEVICE 0

#ifndef M_PI
#define M_PI 3.14159
#endif

#define RUN_ON_CPU

void init_array(int nx, int ny, DATA_TYPE POLYBENCH_2D(A, NX, NY, nx, ny), DATA_TYPE POLYBENCH_1D(p, NY, ny), DATA_TYPE POLYBENCH_1D(r, NX, nx)) {
    int i, j;

    for (i = 0; i < ny; i++) {
        p[i] = i * M_PI;
    }

    for (i = 0; i < nx; i++) {
        r[i] = i * M_PI;

        for (j = 0; j < ny; j++) {
            A[i][j] = ((DATA_TYPE)i * j) / NX;
        }
    }
}

void compareResults(int nx, int ny, DATA_TYPE POLYBENCH_1D(s, NY, ny), DATA_TYPE POLYBENCH_1D(s_outputFromDsp, NY, ny),
                    DATA_TYPE POLYBENCH_1D(q, NX, nx), DATA_TYPE POLYBENCH_1D(q_outputFromDsp, NX, nx)) {
    int i, fail;
    fail = 0;

    // Compare s with s_cuda
    for (i = 0; i < nx; i++) {
        if (percentDiff(q[i], q_outputFromDsp[i]) > PERCENT_DIFF_ERROR_THRESHOLD) {
            fail++;
        }
    }

    for (i = 0; i < ny; i++) {
        if (percentDiff(s[i], s_outputFromDsp[i]) > PERCENT_DIFF_ERROR_THRESHOLD) {
            fail++;
        }
    }
    // Print results
    double fail_percent = (100.0 * fail) / (double)(nx + ny);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void bicg_cpu(int nx, int ny, DATA_TYPE POLYBENCH_2D(A, NX, NY, nx, ny), DATA_TYPE POLYBENCH_1D(r, NX, nx), DATA_TYPE POLYBENCH_1D(s, NY, ny),
              DATA_TYPE POLYBENCH_1D(p, NY, ny), DATA_TYPE POLYBENCH_1D(q, NX, nx)) {
    int i, j;

    for (i = 0; i < _PB_NY; i++) {
        s[i] = 0.0;
    }

    for (i = 0; i < _PB_NX; i++) {
        q[i] = 0.0;
        for (j = 0; j < _PB_NY; j++) {
            s[j] = s[j] + r[i] * A[i][j];
            q[i] = q[i] + A[i][j] * p[j];
        }
    }
}

void print_array(int nx, int ny,
                 DATA_TYPE POLYBENCH_1D(s, NY, ny),
                 DATA_TYPE POLYBENCH_1D(q, NX, nx)) {
    int i;

    for (i = 0; i < ny; i++) {
        fprintf(stderr, DATA_PRINTF_MODIFIER, s[i]);
        if (i % 20 == 0)
            fprintf(stderr, "\n");
    }
    for (i = 0; i < nx; i++) {
        fprintf(stderr, DATA_PRINTF_MODIFIER, q[i]);
        if (i % 20 == 0)
            fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

void bicgDsp(int nx, int ny, DATA_TYPE POLYBENCH_2D(A, NX, NY, nx, ny), DATA_TYPE POLYBENCH_1D(r, NX, nx), DATA_TYPE POLYBENCH_1D(s, NY, ny),
             DATA_TYPE POLYBENCH_1D(p, NY, ny), DATA_TYPE POLYBENCH_1D(q, NX, nx), DATA_TYPE POLYBENCH_1D(s_outputFromDsp, NY, ny),
             DATA_TYPE POLYBENCH_1D(q_outputFromDsp, NX, nx), double *t_DSP_start, double *t_DSP_end) {

    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    // 构建传参数组
    uint64_t args1[5];
    uint64_t args2[5];

    args1[0] = (uint64_t)toRawBits(nx);
    args1[1] = (uint64_t)toRawBits(ny);
    args1[2] = (uint64_t)A;
    args1[3] = (uint64_t)r;
    args1[4] = (uint64_t)s_outputFromDsp;

    args2[0] = (uint64_t)toRawBits(nx);
    args2[1] = (uint64_t)toRawBits(ny);
    args2[2] = (uint64_t)A;
    args2[3] = (uint64_t)p;
    args2[4] = (uint64_t)q_outputFromDsp;

    // 创建线程组并运行
    int groupId = hthread_group_create(clusterId, nthreads);
    // 插入计时开始
    polybench_timer_start(t_DSP_start);

    int rect = hthread_group_exec(groupId, kernel1, 2, 3, args1);
    if (rect != HT_SUCCESS) {
        M_logError("Failed to exec threads with %s", kernel1);
        return;
    }
    hthread_group_wait(groupId);

    rect = hthread_group_exec(groupId, kernel2, 2, 3, args2);
    if (rect != HT_SUCCESS) {
        M_logError("Failed to exec threads with %s", kernel1);
        return;
    }
    hthread_group_wait(groupId);
    // 插入计时结束
    polybench_timer_stop(t_DSP_end);
    hthread_group_destroy(groupId);
}

int main(int argc, char **argv) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    int nx = NX;
    int ny = NY;
    
    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        fprintf(stderr, "DSP initialization failed. Please check hardware and configuration.\n");
        return 0;
    }

    POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, NX, NY, nx, ny, clusterId, HT_MEM_RO);
    POLYBENCH_1D_ARRAY_DECL(s, DATA_TYPE, NY, ny, clusterId, HT_MEM_WO);
    POLYBENCH_1D_ARRAY_DECL(q, DATA_TYPE, NX, nx, clusterId, HT_MEM_WO);
    POLYBENCH_1D_ARRAY_DECL(p, DATA_TYPE, NY, ny, clusterId, HT_MEM_RO);
    POLYBENCH_1D_ARRAY_DECL(r, DATA_TYPE, NX, nx, clusterId, HT_MEM_RO);
    POLYBENCH_1D_ARRAY_DECL(s_outputFromDsp, DATA_TYPE, NY, ny, clusterId, HT_MEM_WO);
    POLYBENCH_1D_ARRAY_DECL(q_outputFromDsp, DATA_TYPE, NX, nx, clusterId, HT_MEM_WO);

    init_array(nx, ny, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(p), POLYBENCH_ARRAY(r));

    bicgDsp(nx, ny, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(r), POLYBENCH_ARRAY(s), POLYBENCH_ARRAY(p), POLYBENCH_ARRAY(q),
            POLYBENCH_ARRAY(s_outputFromDsp), POLYBENCH_ARRAY(q_outputFromDsp), &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU

    /* Start timer. */
    polybench_timer_start(&t_CPU_start);

    bicg_cpu(nx, ny, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(r), POLYBENCH_ARRAY(s), POLYBENCH_ARRAY(p), POLYBENCH_ARRAY(q));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(nx, ny, POLYBENCH_ARRAY(s), POLYBENCH_ARRAY(s_outputFromDsp), POLYBENCH_ARRAY(q),
                   POLYBENCH_ARRAY(q_outputFromDsp));

#else // print output to stderr so no dead code elimination

    print_array(nx, ny, POLYBENCH_ARRAY(s_outputFromGpu), POLYBENCH_ARRAY(q_outputFromGpu));

#endif // RUN_ON_CPU

    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "Data[%d x %d]", nx, ny);
    polybench_PM_print(kernel, clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(A);
    POLYBENCH_FREE_ARRAY(r);
    POLYBENCH_FREE_ARRAY(s);
    POLYBENCH_FREE_ARRAY(p);
    POLYBENCH_FREE_ARRAY(q);
    POLYBENCH_FREE_ARRAY(s_outputFromDsp);
    POLYBENCH_FREE_ARRAY(q_outputFromDsp);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);
    return 0;
}

#include "../../common/polybench.c"
