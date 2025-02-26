
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define POLYBENCH_TIME 1

#include "../../common/MTUtilFuncts.h"
#include "../../common/polybench.h"
#include "corr.h"
#include "hthread_host.h"

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 1.05

#define sqrt_of_array_cell(x, j) sqrt(x[j])

#define FLOAT_N 3214212.01f
#define EPS 0.005f

#define RUN_ON_CPU

void init_arrays(int m, int n, DATA_TYPE POLYBENCH_2D(data, M, N, m, n)) {
    int i, j;

    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            data[i][j] = ((DATA_TYPE)i * j) / M;
        }
    }
}

void correlation(int m, int n, DATA_TYPE POLYBENCH_2D(data, M, N, m, n), DATA_TYPE POLYBENCH_1D(mean, M, m), DATA_TYPE POLYBENCH_1D(stddev, M, m),
                 DATA_TYPE POLYBENCH_2D(symmat, M, N, m, n)) {
    int i, j, j1, j2;

    // Determine mean of column vectors of input data matrix
    for (j = 0; j < _PB_M; j++) {
        mean[j] = 0.0;

        for (i = 0; i < _PB_N; i++) {
            mean[j] += data[i][j];
        }

        mean[j] /= (DATA_TYPE)FLOAT_N;
    }

    // Determine standard deviations of column vectors of data matrix.
    for (j = 0; j < _PB_M; j++) {
        stddev[j] = 0.0;

        for (i = 0; i < _PB_N; i++) {
            stddev[j] += (data[i][j] - mean[j]) * (data[i][j] - mean[j]);
        }

        stddev[j] /= FLOAT_N;
        stddev[j] = sqrt_of_array_cell(stddev, j);
        stddev[j] = stddev[j] <= EPS ? 1.0 : stddev[j];
    }

    // Center and reduce the column vectors.
    for (i = 0; i < _PB_N; i++) {
        for (j = 0; j < _PB_M; j++) {
            data[i][j] -= mean[j];
            data[i][j] /= (sqrt(FLOAT_N) * stddev[j]);
        }
    }

    // Calculate the m * m correlation matrix.
    for (j1 = 0; j1 < _PB_M - 1; j1++) {
        symmat[j1][j1] = 1.0;

        for (j2 = j1 + 1; j2 < _PB_M; j2++) {
            symmat[j1][j2] = 0.0;

            for (i = 0; i < _PB_N; i++) {
                symmat[j1][j2] += (data[i][j1] * data[i][j2]);
            }

            symmat[j2][j1] = symmat[j1][j2];
        }
    }

    symmat[M - 1][M - 1] = 1.0;
}

void compareResults(int m, int n, DATA_TYPE POLYBENCH_2D(symmat, M, N, m, n), DATA_TYPE POLYBENCH_2D(symmat_outputFromDsp, M, N, m, n)) {
    int i, j, fail;
    fail = 0;

    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            if (percentDiff(symmat[i][j], symmat_outputFromDsp[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }
    // Print results
    double fail_percent = (100.0 * fail) / (double)(m * n);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

void correlationDsp(int m, int n, DATA_TYPE POLYBENCH_2D(data, M, N, m, n), DATA_TYPE POLYBENCH_1D(mean, M, m),
                    DATA_TYPE POLYBENCH_1D(stddev, M, m), DATA_TYPE POLYBENCH_2D(symmat, M, N, m, n),
                    DATA_TYPE POLYBENCH_2D(symmat_outputFromDsp, M, N, m, n), double *t_DSP_start, double *t_DSP_end) {
    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    // 构建传参数组
    uint64_t args0[4];
    uint64_t args1[5];
    uint64_t args2[5];
    uint64_t args3[4];

    args0[0] = (uint64_t)toRawBits(m);
    args0[1] = (uint64_t)toRawBits(n);
    args0[2] = (uint64_t)mean;
    args0[3] = (uint64_t)data;

    args1[0] = (uint64_t)toRawBits(m);
    args1[1] = (uint64_t)toRawBits(n);
    args1[2] = (uint64_t)mean;
    args1[3] = (uint64_t)stddev;
    args1[4] = (uint64_t)data;

    args2[0] = (uint64_t)toRawBits(m);
    args2[1] = (uint64_t)toRawBits(n);
    args2[2] = (uint64_t)mean;
    args2[3] = (uint64_t)stddev;
    args2[4] = (uint64_t)data;

    args3[0] = (uint64_t)toRawBits(m);
    args3[1] = (uint64_t)toRawBits(n);
    args3[2] = (uint64_t)symmat_outputFromDsp;
    args3[3] = (uint64_t)data;

    // 创建线程组并运行
    int groupId = hthread_group_create(clusterId, nthreads);
    polybench_timer_start(t_DSP_start);

    hthread_group_exec(groupId, kernel0, 2, 2, args0);
    hthread_group_wait(groupId);
    hthread_group_exec(groupId, kernel1, 2, 3, args1);
    hthread_group_wait(groupId);
    hthread_group_exec(groupId, kernel2, 2, 3, args2);
    hthread_group_wait(groupId);
    hthread_group_exec(groupId, kernel3, 2, 2, args3);
    hthread_group_wait(groupId);

    // 插入计时结束
    polybench_timer_stop(t_DSP_end);
    hthread_group_destroy(groupId);
}

void print_array(int m, DATA_TYPE POLYBENCH_2D(symmat, M, M, m, m)) {
    int i, j;

    for (i = 0; i < m; i++)
        for (j = 0; j < m; j++) {
            fprintf(stderr, DATA_PRINTF_MODIFIER, symmat[i][j]);
            if ((i * m + j) % 20 == 0)
                fprintf(stderr, "\n");
        }
    fprintf(stderr, "\n");
}

int main() {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    int m = M;
    int n = N;

    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        fprintf(stderr, "DSP initialization failed. Please check hardware and configuration.\n");
        return 0;
    }

    POLYBENCH_2D_ARRAY_DECL(data, DATA_TYPE, M, N, m, n, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(mean, DATA_TYPE, M, m, clusterId, HT_MEM_RW);
    POLYBENCH_1D_ARRAY_DECL(stddev, DATA_TYPE, M, m, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(symmat, DATA_TYPE, M, N, m, n, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(symmat_outputFromDsp, DATA_TYPE, M, N, m, n, clusterId, HT_MEM_RW);

    init_arrays(m, n, POLYBENCH_ARRAY(data));

    correlationDsp(m, n, POLYBENCH_ARRAY(data), POLYBENCH_ARRAY(mean), POLYBENCH_ARRAY(stddev), POLYBENCH_ARRAY(symmat),
                   POLYBENCH_ARRAY(symmat_outputFromDsp), &t_DSP_start, &t_DSP_end);

#ifdef RUN_ON_CPU
    init_arrays(m, n, POLYBENCH_ARRAY(data));

    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);

    correlation(m, n, POLYBENCH_ARRAY(data), POLYBENCH_ARRAY(mean), POLYBENCH_ARRAY(stddev), POLYBENCH_ARRAY(symmat));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(m, n, POLYBENCH_ARRAY(symmat), POLYBENCH_ARRAY(symmat_outputFromDsp));

#else // print output to stderr so no dead code elimination

    print_array(m, POLYBENCH_ARRAY(symmat_outputFromDsp));

#endif // RUN_ON_CPU

    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "Data[%d x %d], Mean[%d], Std[%d]", m, n, m, m);
    polybench_PM_print(kernel3, clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(data);
    POLYBENCH_FREE_ARRAY(mean);
    POLYBENCH_FREE_ARRAY(stddev);
    POLYBENCH_FREE_ARRAY(symmat);
    POLYBENCH_FREE_ARRAY(symmat_outputFromDsp);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);
    return 0;
}

#include "../../common/polybench.c"
