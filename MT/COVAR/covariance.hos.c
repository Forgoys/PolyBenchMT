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
#include "covariance.h"
#include "hthread_host.h"

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 1.05

#define sqrt_of_array_cell(x, j) sqrt(x[j])

#define FLOAT_N 3214212.01
#define EPS 0.005

#define RUN_ON_CPU

void init_arrays(int m, int n, DATA_TYPE POLYBENCH_2D(data, M, N, m, n)) {
    int i, j;

    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            data[i][j] = ((DATA_TYPE)i * j) / M;
        }
    }
}

void covariance(int m, int n, DATA_TYPE POLYBENCH_2D(data, M, N, m, n), DATA_TYPE POLYBENCH_2D(symmat, M, M, m, m), DATA_TYPE POLYBENCH_1D(mean, M, m)) {
    int i, j, j1, j2;

    /* Determine mean of column vectors of input data matrix */
    for (j = 0; j < _PB_M; j++) {
        mean[j] = 0.0;
        for (i = 0; i < _PB_N; i++) {
            mean[j] += data[i][j];
        }
        mean[j] /= FLOAT_N;
    }

    /* Center the column vectors. */
    for (i = 0; i < _PB_N; i++) {
        for (j = 0; j < _PB_M; j++) {
            data[i][j] -= mean[j];
        }
    }

    /* Calculate the m * m covariance matrix. */
    for (j1 = 0; j1 < _PB_M; j1++) {
        for (j2 = j1; j2 < _PB_M; j2++) {
            symmat[j1][j2] = 0.0;
            for (i = 0; i < _PB_N; i++) {
                symmat[j1][j2] += data[i][j1] * data[i][j2];
            }
            symmat[j2][j1] = symmat[j1][j2];
        }
    }
}

void compareResults(int m, int n, DATA_TYPE POLYBENCH_2D(symmat, M, M, m, m), DATA_TYPE POLYBENCH_2D(symmat_outputFromDSP, M, M, m, m)) {
    int i, j, fail;
    fail = 0;

    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            if (percentDiff(symmat[i][j], symmat_outputFromDSP[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }

    double fail_percent = (100.0 * fail) / (double)(m * n);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void covarianceDSP(int m, int n, DATA_TYPE POLYBENCH_2D(data, M, N, m, n), DATA_TYPE POLYBENCH_2D(symmat, M, M, m, m), DATA_TYPE POLYBENCH_1D(mean, M, m),
                   DATA_TYPE POLYBENCH_2D(symmat_outputFromDSP, M, M, m, m), double *t_DSP_start, double *t_DSP_end) {
    memcpy(symmat_outputFromDSP, symmat, sizeof(DATA_TYPE) * M * M);

    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    // 非指针变量需要使用toRawBits转换
    uint64_t args1[4];
    args1[0] = (uint64_t)toRawBits(m);
    args1[1] = (uint64_t)toRawBits(n);
    args1[2] = (uint64_t)mean;
    args1[3] = (uint64_t)data;

    uint64_t args2[4];
    args2[0] = (uint64_t)toRawBits(m);
    args2[1] = (uint64_t)toRawBits(n);
    args2[2] = (uint64_t)symmat_outputFromDSP;
    args2[3] = (uint64_t)data;

    polybench_timer_start(t_DSP_start);

    // 创建线程组
    int groupId = hthread_group_create(clusterId, nthreads);
    // 验证线程组创建结果
    if (groupId == -1) {
        M_logError("Failed to create group with %d threads.", nthreads);
        return;
    }

    polybench_timer_start(t_DSP_start);

    hthread_group_exec(groupId, kernel1, 2, 2, args1);
    hthread_group_wait(groupId);
    hthread_group_exec(groupId, kernel2, 2, 2, args1);
    hthread_group_wait(groupId);
    hthread_group_exec(groupId, kernel3, 2, 2, args2);
    hthread_group_wait(groupId);

    polybench_timer_stop(t_DSP_end);
    hthread_group_destroy(groupId);
}

static void print_array(int m, DATA_TYPE POLYBENCH_2D(symmat, M, M, m, m)) {
    int i, j;

    for (i = 0; i < m; i++)
        for (j = 0; j < m; j++) {
            fprintf(stderr, DATA_PRINTF_MODIFIER, symmat[i][j]);
            if ((i * m + j) % 20 == 0)
                fprintf(stderr, "\n");
        }
    fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    int m = M;
    int n = N;

    /* DSP device init. */
    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        return 0;
    }

    POLYBENCH_2D_ARRAY_DECL(data, DATA_TYPE, M, N, m, n, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(symmat, DATA_TYPE, M, M, m, m, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(mean, DATA_TYPE, M, m, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(symmat_outputFromDSP, DATA_TYPE, M, M, m, m, clusterId, HT_MEM_RW);

    init_arrays(m, n, POLYBENCH_ARRAY(data));

    covarianceDSP(m, n, POLYBENCH_ARRAY(data), POLYBENCH_ARRAY(symmat), POLYBENCH_ARRAY(mean), POLYBENCH_ARRAY(symmat_outputFromDSP), &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU

    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);
    covariance(m, n, POLYBENCH_ARRAY(data), POLYBENCH_ARRAY(symmat), POLYBENCH_ARRAY(mean));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(m, n, POLYBENCH_ARRAY(symmat), POLYBENCH_ARRAY(symmat_outputFromDSP));

#else // print output to stderr so no dead code elimination

    print_array(n, POLYBENCH_ARRAY(a_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "data[%d x %d]", M, N);
    polybench_PM_print("covariance", clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(data);
    POLYBENCH_FREE_ARRAY(symmat);
    POLYBENCH_FREE_ARRAY(mean);
    POLYBENCH_FREE_ARRAY(symmat_outputFromDSP);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);
    return 0;
}

#include "../../common/polybench.c"