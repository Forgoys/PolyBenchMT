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
#include "3mm.h"
#include "hthread_host.h"

// define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05
#define DSP_DEVICE 0

#define RUN_ON_CPU

void init_array(int ni, int nj, int nk, int nl, int nm, DATA_TYPE POLYBENCH_2D(A, NI, NK, ni, nk), DATA_TYPE POLYBENCH_2D(B, NK, NJ, nk, nj),
                DATA_TYPE POLYBENCH_2D(C, NJ, NM, nj, nm), DATA_TYPE POLYBENCH_2D(D, NM, NL, nm, nl)) {
    int i, j;

    for (i = 0; i < ni; i++) {
        for (j = 0; j < nk; j++) {
            A[i][j] = ((DATA_TYPE)i * j) / ni;
        }
    }

    for (i = 0; i < nk; i++) {
        for (j = 0; j < nj; j++) {
            B[i][j] = ((DATA_TYPE)i * (j + 1)) / nj;
        }
    }

    for (i = 0; i < nj; i++) {
        for (j = 0; j < nm; j++) {
            C[i][j] = ((DATA_TYPE)i * (j + 3)) / nl;
        }
    }

    for (i = 0; i < nm; i++) {
        for (j = 0; j < nl; j++) {
            D[i][j] = ((DATA_TYPE)i * (j + 2)) / nk;
        }
    }
}

void compareResults(int ni, int nl, DATA_TYPE POLYBENCH_2D(G, NI, NL, ni, nl), DATA_TYPE POLYBENCH_2D(G_outputFromDSP, NI, NL, ni, nl)) {
    int i, j, fail;
    fail = 0;

    for (i = 0; i < ni; i++) {
        for (j = 0; j < nl; j++) {
            if (percentDiff(G[i][j], G_outputFromDSP[i][j]) > PERCENT_DIFF_ERROR_THRESHOLD) {
                fail++;
            }
        }
    }

    // print results
    double fail_percent = (100.0 * fail) / (double)(ni * nl);
    printf("Non-Matching CPU-DSP Outputs Beyond Error Threshold of %4.2f Percent: %d (%.2lf%%)\n",
           PERCENT_DIFF_ERROR_THRESHOLD, fail, fail_percent);
}

/* Main computational kernel on CPU */
void mm3(int ni, int nj, int nk, int nl, int nm,
         DATA_TYPE POLYBENCH_2D(E, NI, NJ, ni, nj),
         DATA_TYPE POLYBENCH_2D(A, NI, NK, ni, nk),
         DATA_TYPE POLYBENCH_2D(B, NK, NJ, nk, nj),
         DATA_TYPE POLYBENCH_2D(F, NJ, NL, nj, nl),
         DATA_TYPE POLYBENCH_2D(C, NJ, NM, nj, nm),
         DATA_TYPE POLYBENCH_2D(D, NM, NL, nm, nl),
         DATA_TYPE POLYBENCH_2D(G, NI, NL, ni, nl)) {
    int i, j, k;

    /* E := A*B */
    for (i = 0; i < _PB_NI; i++) {
        for (j = 0; j < _PB_NJ; j++) {
            E[i][j] = 0;
            for (k = 0; k < _PB_NK; ++k) {
                E[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    /* F := C*D */
    for (i = 0; i < _PB_NJ; i++) {
        for (j = 0; j < _PB_NL; j++) {
            F[i][j] = 0;
            for (k = 0; k < _PB_NM; ++k) {
                F[i][j] += C[i][k] * D[k][j];
            }
        }
    }

    /* G := E*F */
    for (i = 0; i < _PB_NI; i++) {
        for (j = 0; j < _PB_NL; j++) {
            G[i][j] = 0;
            for (k = 0; k < _PB_NJ; ++k) {
                G[i][j] += E[i][k] * F[k][j];
            }
        }
    }
}

void mm3DSP(int ni, int nj, int nk, int nl, int nm,
            DATA_TYPE POLYBENCH_2D(E, NI, NJ, ni, nj),
            DATA_TYPE POLYBENCH_2D(A, NI, NK, ni, nk),
            DATA_TYPE POLYBENCH_2D(B, NK, NJ, nk, nj),
            DATA_TYPE POLYBENCH_2D(F, NJ, NL, nj, nl),
            DATA_TYPE POLYBENCH_2D(C, NJ, NM, nj, nm),
            DATA_TYPE POLYBENCH_2D(D, NM, NL, nm, nl),
            DATA_TYPE POLYBENCH_2D(G, NI, NL, ni, nl),
            DATA_TYPE POLYBENCH_2D(G_outputFromDSP, NI, NL, ni, nl),
            double *t_DSP_start, double *t_DSP_end) {
    memcpy(G_outputFromDSP, G, sizeof(DATA_TYPE) * NI * NL);

    // 打开设备并载入dat文件
    int retc = hthread_dev_open(clusterId);
    M_checkRetC(retc, hthread_dev_open);
    retc = hthread_dat_load(clusterId, devProgram);
    M_checkRetC(retc, hthread_dat_load);

    uint64_t args1[8];
    args1[0] = (uint64_t)toRawBits(ni);
    args1[1] = (uint64_t)toRawBits(nj);
    args1[2] = (uint64_t)toRawBits(nk);
    args1[3] = (uint64_t)toRawBits(nl);
    args1[4] = (uint64_t)toRawBits(nm);
    args1[5] = (uint64_t)A;
    args1[6] = (uint64_t)B;
    args1[7] = (uint64_t)E;

    uint64_t args2[8];
    args2[0] = (uint64_t)toRawBits(ni);
    args2[1] = (uint64_t)toRawBits(nj);
    args2[2] = (uint64_t)toRawBits(nk);
    args2[3] = (uint64_t)toRawBits(nl);
    args2[4] = (uint64_t)toRawBits(nm);
    args2[5] = (uint64_t)C;
    args2[6] = (uint64_t)D;
    args2[7] = (uint64_t)F;

    uint64_t args3[8];
    args3[0] = (uint64_t)toRawBits(ni);
    args3[1] = (uint64_t)toRawBits(nj);
    args3[2] = (uint64_t)toRawBits(nk);
    args3[3] = (uint64_t)toRawBits(nl);
    args3[4] = (uint64_t)toRawBits(nm);
    args3[5] = (uint64_t)E;
    args3[6] = (uint64_t)F;
    args3[7] = (uint64_t)G_outputFromDSP;

    // 创建线程组
    int groupId = hthread_group_create(clusterId, nthreads);
    // 验证线程组创建结果
    if (groupId == -1) {
        M_logError("Failed to create group with %d threads.", nthreads);
        return;
    }

    // 插入计时开始代码
    polybench_timer_start(t_DSP_start);

    hthread_group_exec(groupId, kernel1, 5, 3, args1);
    hthread_group_wait(groupId);
    hthread_group_exec(groupId, kernel2, 5, 3, args2);
    hthread_group_wait(groupId);
    hthread_group_exec(groupId, kernel3, 5, 3, args3);
    hthread_group_wait(groupId);

    // 插入计时结束代码
    polybench_timer_stop(t_DSP_end);
    hthread_group_destroy(groupId);
}

static void print_array(int ni, int nl,
                        DATA_TYPE POLYBENCH_2D(G, NI, NL, ni, nl)) {
    int i, j;

    for (i = 0; i < ni; i++)
        for (j = 0; j < nl; j++) {
            fprintf(stderr, DATA_PRINTF_MODIFIER, G[i][j]);
            if ((i * ni + j) % 20 == 0)
                fprintf(stderr, "\n");
        }
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[]) {
    double t_DSP_start, t_DSP_end, t_CPU_start, t_CPU_end;

    int ni = NI;
    int nj = NJ;
    int nk = NK;
    int nl = NL;
    int nm = NM;

    /* DSP device init. */
    int clusterId = 0;
    int nthreads = THREAD_NUM;
    clusterId = DSP_init(devProgram, nthreads);
    if (clusterId == -1) {
        return 0;
    }

    /* Variable declaration/allocation. */
    POLYBENCH_2D_ARRAY_DECL(E, DATA_TYPE, NI, NJ, ni, nj, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, NI, NK, ni, nk, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(B, DATA_TYPE, NK, NJ, nk, nj, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(F, DATA_TYPE, NJ, NL, nj, nl, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(C, DATA_TYPE, NJ, NM, nj, nm, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(D, DATA_TYPE, NM, NL, nm, nl, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(G, DATA_TYPE, NI, NL, ni, nl, clusterId, HT_MEM_RW);
    POLYBENCH_2D_ARRAY_DECL(G_outputFromDSP, DATA_TYPE, NI, NL, ni, nl, clusterId, HT_MEM_RW);

    init_array(ni, nj, nk, nl, nm, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C), POLYBENCH_ARRAY(D));

    mm3DSP(ni, nj, nk, nl, nm, POLYBENCH_ARRAY(E), POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(F), POLYBENCH_ARRAY(C),
           POLYBENCH_ARRAY(D), POLYBENCH_ARRAY(G), POLYBENCH_ARRAY(G_outputFromDSP), &t_DSP_start, &t_DSP_end);
#ifdef RUN_ON_CPU

    init_array(ni, nj, nk, nl, nm, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C), POLYBENCH_ARRAY(D));
    /* Start CPU timer. */
    polybench_timer_start(&t_CPU_start);

    mm3(ni, nj, nk, nl, nm, POLYBENCH_ARRAY(E), POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(F), POLYBENCH_ARRAY(C),
        POLYBENCH_ARRAY(D), POLYBENCH_ARRAY(G));

    /* Stop CPU timer. */
    polybench_timer_stop(&t_CPU_end);

    compareResults(ni, nl, POLYBENCH_ARRAY(G), POLYBENCH_ARRAY(G_outputFromDSP));

#else

    print_array(n, POLYBENCH_ARRAY(X_outputFromDsp));

#endif // RUN_ON_CPU

    /* print performance metrics */
    /* 构建负载信息字符串 */
    char workload_info[100];
    sprintf(workload_info, "NI:%d, NJ:%d, NK:%d, NL:%d, NM:%d", ni, nj, nk, nl, nm);
    polybench_PM_print("3mm", clusterId, nthreads, t_CPU_start, t_CPU_end, t_DSP_start, t_DSP_end, workload_info);

    POLYBENCH_FREE_ARRAY(A);
    POLYBENCH_FREE_ARRAY(B);
    POLYBENCH_FREE_ARRAY(C);
    POLYBENCH_FREE_ARRAY(D);
    POLYBENCH_FREE_ARRAY(E);
    POLYBENCH_FREE_ARRAY(F);
    POLYBENCH_FREE_ARRAY(G);
    POLYBENCH_FREE_ARRAY(G_outputFromDSP);

    int retc = hthread_dat_unload(clusterId);
    M_checkRetC(retc, hthread_dat_unload);
    retc = hthread_dev_close(clusterId);
    M_checkRetC(retc, hthread_dev_close);

    return 0;
}

#include "../../common/polybench.c"
