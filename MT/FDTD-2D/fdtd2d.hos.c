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
#include "fdtd2d.h"
#include "hthread_host.h"

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 10.05

#define RUN_ON_CPU

void init_arrays(int tmax, int nx, int ny, DATA_TYPE POLYBENCH_1D(_fict_, TMAX, TMAX), DATA_TYPE POLYBENCH_2D(ex, NX, NY, nx, ny),
                 DATA_TYPE POLYBENCH_2D(ey, NX, NY, nx, ny), DATA_TYPE POLYBENCH_2D(hz, NX, NY, nx, ny)) {
    int i, j;

    for (i = 0; i < tmax; i++) {
        _fict_[i] = (DATA_TYPE)i;
    }

    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny; j++) {
            ex[i][j] = ((DATA_TYPE)i * (j + 1) + 1) / NX;
            ey[i][j] = ((DATA_TYPE)(i - 1) * (j + 2) + 2) / NX;
            hz[i][j] = ((DATA_TYPE)(i - 9) * (j + 4) + 3) / NX;
        }
    }
}

void fdtd2d(int tmax, int nx, int ny, DATA_TYPE POLYBENCH_1D(_fict_, TMAX, TMAX), DATA_TYPE POLYBENCH_2D(ex, NX, NY, nx, ny),
            DATA_TYPE POLYBENCH_2D(ey, NX, NY, nx, ny), DATA_TYPE POLYBENCH_2D(hz, NX, NY, nx, ny)) {
    int t, i, j;

    for (t = 0; t < _PB_TMAX; t++) {
        for (j = 0; j < _PB_NY; j++) {
            ey[0][j] = _fict_[t];
        }

        for (i = 1; i < _PB_NX; i++) {
            for (j = 0; j < _PB_NY; j++) {
                ey[i][j] = ey[i][j] - 0.5 * (hz[i][j] - hz[(i - 1)][j]);
            }
        }

        for (i = 0; i < _PB_NX; i++) {
            for (j = 1; j < _PB_NY; j++) {
                ex[i][j] = ex[i][j] - 0.5 * (hz[i][j] - hz[i][(j - 1)]);
            }
        }

        for (i = 0; i < _PB_NX - 1; i++) {
            for (j = 0; j < _PB_NY - 1; j++) {
                hz[i][j] = hz[i][j] - 0.7 * (ex[i][(j + 1)] - ex[i][j] + ey[(i + 1)][j] - ey[i][j]);
            }
        }
    }
}

void compareResults(int nx, int ny, DATA_TYPE POLYBENCH_2D(hz1, NX, NY, nx, ny), DATA_TYPE POLYBENCH_2D(hz2, NX, NY, nx, ny)) {
    int i, j, fail;
    fail = 0;

    for (i = 0; i < nx; i++) {
        for (j = 0; j < ny; j++) {
            if (percentDiff(hz1[i][j], hz2[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }

    // Print results
    double fail_percent = (100.0 * fail) / (double)(nx * ny);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void fdtd2dDSP(int tmax, int nx, int ny, DATA_TYPE POLYBENCH_1D(_fict_, TMAX, TMAX), DATA_TYPE POLYBENCH_2D(ex, NX, NY, nx, ny),
               DATA_TYPE POLYBENCH_2D(ey, NX, NY, nx, ny), DATA_TYPE POLYBENCH_2D(hz, NX, NY, nx, ny), DATA_TYPE POLYBENCH_2D(hz_outputFromDSP, NX, NY, nx, ny), double *t_DSP_start, double *t_DSP_end) {
    memcpy(hz_outputFromDSP, hz, sizeof(DATA_TYPE) * NX * NY);

    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    // 非指针变量需要使用toRawBits转换
    uint64_t args1[6];
    args1[0] = (uint64_t)toRawBits(nx); // 非指针变量，需要toRawBits转换
    args1[1] = (uint64_t)toRawBits(ny); // 非指针变量，需要toRawBits转换
    // args1[2] = (uint64_t)toRawBits(t);    // 非指针变量，需要toRawBits转换
    args1[3] = (uint64_t)ex;               // 指针变量，直接转换为uint64_t
    args1[4] = (uint64_t)ey;               // 指针变量，直接转换为uint64_t
    args1[5] = (uint64_t)hz_outputFromDSP; // 指针变量，直接转换为uint64_t

    uint64_t args2[7];
    args2[0] = (uint64_t)toRawBits(nx); // 非指针变量，需要toRawBits转换
    args2[1] = (uint64_t)toRawBits(ny); // 非指针变量，需要toRawBits转换
    // args2[2] = (uint64_t)toRawBits(t);    // 非指针变量，需要toRawBits转换
    args2[3] = (uint64_t)_fict_;           // 指针变量，直接转换为uint64_t
    args2[4] = (uint64_t)ex;               // 指针变量，直接转换为uint64_t
    args2[5] = (uint64_t)ey;               // 指针变量，直接转换为uint64_t
    args2[6] = (uint64_t)hz_outputFromDSP; // 指针变量，直接转换为uint64_t

    polybench_timer_start(t_DSP_start);

    // 创建线程组
    int groupId = hthread_group_create(clusterId, nthreads);
    // 验证线程组创建结果
    if (groupId == -1) {
        M_logError("Failed to create group with %d threads.", nthreads);
        return;
    }

    polybench_timer_start(t_DSP_start);

    for (int t = 0; t < _PB_TMAX; t++) {
        args1[2] = (uint64_t)toRawBits(t); // 每次迭代更新i1
        args2[2] = (uint64_t)toRawBits(t); // 每次迭代更新i1
        hthread_group_exec(groupId, kernel1, 3, 4, args2);
        hthread_group_wait(groupId);
        hthread_group_exec(groupId, kernel2, 3, 3, args1);
        hthread_group_wait(groupId);
        hthread_group_exec(groupId, kernel3, 3, 3, args1);
        hthread_group_wait(groupId);
    }

    polybench_timer_stop(t_DSP_end);
    hthread_group_destroy(groupId);
}

/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static void print_array(int nx, int ny, DATA_TYPE POLYBENCH_2D(hz, NX, NY, nx, ny)) {
    int i, j;

    for (i = 0; i < nx; i++)
        for (j = 0; j < ny; j++) {
            fprintf(stderr, DATA_PRINTF_MODIFIER, hz[i][j]);
            if ((i * nx + j) % 20 == 0)
                fprintf(stderr, "\n");
        }
    fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    /* Retrieve problem size. */
    int tmax = TMAX;
    int nx = NX;
    int ny = NY;

    /* DSP device init. */
    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        return 0;
    }

    POLYBENCH_1D_ARRAY_DECL(_fict_, DATA_TYPE, TMAX, TMAX, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(ex, DATA_TYPE, NX, NY, nx, ny, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(ey, DATA_TYPE, NX, NY, nx, ny, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(hz, DATA_TYPE, NX, NY, nx, ny, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(hz_outputFromDsp, DATA_TYPE, NX, NY, nx, ny, clusterId, HT_MEM_RW);

    init_arrays(tmax, nx, ny, POLYBENCH_ARRAY(_fict_), POLYBENCH_ARRAY(ex), POLYBENCH_ARRAY(ey), POLYBENCH_ARRAY(hz));

    fdtd2dDSP(tmax, nx, ny, POLYBENCH_ARRAY(_fict_), POLYBENCH_ARRAY(ex), POLYBENCH_ARRAY(ey), POLYBENCH_ARRAY(hz), POLYBENCH_ARRAY(hz_outputFromDsp), &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU
    init_arrays(tmax, nx, ny, POLYBENCH_ARRAY(_fict_), POLYBENCH_ARRAY(ex), POLYBENCH_ARRAY(ey), POLYBENCH_ARRAY(hz));

    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);
    fdtd2d(tmax, nx, ny, POLYBENCH_ARRAY(_fict_), POLYBENCH_ARRAY(ex), POLYBENCH_ARRAY(ey), POLYBENCH_ARRAY(hz));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(nx, ny, POLYBENCH_ARRAY(hz), POLYBENCH_ARRAY(hz_outputFromDsp));

#else // print output to stderr so no dead code elimination

    print_array(n, POLYBENCH_ARRAY(a_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "Grid[%d x %d], tmax: %d", NX, NY, TMAX);
    polybench_PM_print("fdtd2d", clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(_fict_);
    POLYBENCH_FREE_ARRAY(ex);
    POLYBENCH_FREE_ARRAY(ey);
    POLYBENCH_FREE_ARRAY(hz);
    POLYBENCH_FREE_ARRAY(hz_outputFromDsp);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);
    return 0;
}

#include "../../common/polybench.c"
