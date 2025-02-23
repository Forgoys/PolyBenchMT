#include "adi.h"
#include "../../common/polybench.h"
#include "hthread_device.h"
#include <compiler/m3000.h>

__global__ void adi_kernel1(int n, DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *X) {
    int thread_id = get_thread_id();
    int total_threads = get_group_size();

    // 计算每个线程需要处理的行数
    int rows_per_thread = _PB_N / total_threads;
    int extra_rows = _PB_N % total_threads;

    // 计算当前线程的起始行和结束行
    int start_row = thread_id * rows_per_thread + (thread_id < extra_rows ? thread_id : extra_rows);
    int end_row = start_row + rows_per_thread + (thread_id < extra_rows ? 1 : 0);
    // 处理分配给当前线程的行
    for (int i1 = start_row; i1 < end_row; i1++) {
        for (int i2 = 1; i2 < _PB_N; i2++) {
            X[i1 * N + i2] = X[i1 * N + i2] - X[i1 * N + (i2 - 1)] * A[i1 * N + i2] / B[i1 * N + (i2 - 1)];
            B[i1 * N + i2] = B[i1 * N + i2] - A[i1 * N + i2] * A[i1 * N + i2] / B[i1 * N + (i2 - 1)];
        }
    }
}

__global__ void adi_kernel2(int n, DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *X) {
    int thread_id = get_thread_id();
    int total_threads = get_group_size();

    // 计算每个线程需要处理的行数
    int rows_per_thread = _PB_N / total_threads;
    int extra_rows = _PB_N % total_threads;

    // 计算当前线程的起始行和结束行
    int start_row = thread_id * rows_per_thread + (thread_id < extra_rows ? thread_id : extra_rows);
    int end_row = start_row + rows_per_thread + (thread_id < extra_rows ? 1 : 0);

    // 处理分配给当前线程的行
    for (int i1 = start_row; i1 < end_row; i1++) {
        X[i1 * N + (N - 1)] = X[i1 * N + (N - 1)] / B[i1 * N + (N - 1)];
    }
}

__global__ void adi_kernel3(int n, DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *X) {
    int thread_id = get_thread_id();
    int total_threads = get_group_size();

    // 计算每个线程需要处理的行数
    int rows_per_thread = _PB_N / total_threads;
    int extra_rows = _PB_N % total_threads;

    // 计算当前线程的起始行和结束行
    int start_row = thread_id * rows_per_thread + (thread_id < extra_rows ? thread_id : extra_rows);
    int end_row = start_row + rows_per_thread + (thread_id < extra_rows ? 1 : 0);

    // 处理分配给当前线程的行
    for (int i1 = start_row; i1 < end_row; i1++) {
        for (int i2 = 0; i2 < _PB_N - 2; i2++) {
            X[i1 * N + (N - i2 - 2)] = (X[i1 * N + (N - 2 - i2)] - X[i1 * N + (N - 2 - i2 - 1)] * A[i1 * N + (N - i2 - 3)]) / B[i1 * N + (N - 3 - i2)];
        }
    }
}

__global__ void adi_kernel4(int n, int i1, DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *X) {
    int thread_id = get_thread_id();
    int total_threads = get_group_size();

    // 计算每个线程需要处理的列数
    int cols_per_thread = _PB_N / total_threads;
    int extra_cols = _PB_N % total_threads;

    // 计算当前线程的起始列和结束列
    int start_col = thread_id * cols_per_thread + (thread_id < extra_cols ? thread_id : extra_cols);
    int end_col = start_col + cols_per_thread + (thread_id < extra_cols ? 1 : 0);

    // 处理分配给当前线程的列
    for (int i2 = start_col; i2 < end_col; i2++) {
        X[i1 * N + i2] = X[i1 * N + i2] - X[(i1 - 1) * N + i2] * A[i1 * N + i2] / B[(i1 - 1) * N + i2];
        B[i1 * N + i2] = B[i1 * N + i2] - A[i1 * N + i2] * A[i1 * N + i2] / B[(i1 - 1) * N + i2];
    }
}

__global__ void adi_kernel5(int n, DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *X) {
    int thread_id = get_thread_id();
    int total_threads = get_group_size();

    // 计算每个线程需要处理的列数
    int cols_per_thread = _PB_N / total_threads;
    int extra_cols = _PB_N % total_threads;

    // 计算当前线程的起始列和结束列
    int start_col = thread_id * cols_per_thread + (thread_id < extra_cols ? thread_id : extra_cols);
    int end_col = start_col + cols_per_thread + (thread_id < extra_cols ? 1 : 0);

    // 处理分配给当前线程的列
    for (int i2 = start_col; i2 < end_col; i2++) {
        X[(N - 1) * N + i2] = X[(N - 1) * N + i2] / B[(N - 1) * N + i2];
    }
}

__global__ void adi_kernel6(int n, int i1, DATA_TYPE *A, DATA_TYPE *B, DATA_TYPE *X) {
    int thread_id = get_thread_id();
    int total_threads = get_group_size();

    // 计算每个线程需要处理的列数
    int cols_per_thread = _PB_N / total_threads;
    int extra_cols = _PB_N % total_threads;

    // 计算当前线程的起始列和结束列
    int start_col = thread_id * cols_per_thread + (thread_id < extra_cols ? thread_id : extra_cols);
    int end_col = start_col + cols_per_thread + (thread_id < extra_cols ? 1 : 0);
    // hthread_printf("thread %d, start_col: %d, end_col: %d\n", thread_id, start_col, end_col);
    // 处理分配给当前线程的列
    for (int i2 = start_col; i2 < end_col; i2++) {
        X[(N - 2 - i1) * N + i2] = (X[(N - 2 - i1) * N + i2] - X[(N - i1 - 3) * N + i2] * A[(N - 3 - i1) * N + i2]) / B[(N - 2 - i1) * N + i2];
    }
}