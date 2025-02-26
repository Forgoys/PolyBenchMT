#ifndef CORRELATION_H
#define CORRELATION_H

/* Default to STANDARD_DATASET. */
#if !defined(MINI_DATASET) && !defined(SMALL_DATASET) && !defined(LARGE_DATASET) && !defined(EXTRALARGE_DATASET)
#define STANDARD_DATASET
#endif

/* Do not define anything if the user manually defines the size. */
#if !defined(N)
/* Define the possible dataset sizes. */
#ifdef MINI_DATASET
#define M 512
#define N 512
#endif

#ifdef SMALL_DATASET
#define M 640
#define N 640
#endif

#ifdef STANDARD_DATASET /* Default if unspecified. */
#define M 768
#define N 768
#endif

#ifdef LARGE_DATASET
#define M 896
#define N 896
#endif

#ifdef EXTRALARGE_DATASET
#define M 1024
#define N 1024
#endif
#endif /* !N */

#define _PB_M POLYBENCH_LOOP_BOUND(M, m)
#define _PB_N POLYBENCH_LOOP_BOUND(N, n)

#ifndef DATA_TYPE
#define DATA_TYPE double
#define DATA_PRINTF_MODIFIER "%0.2lf "
#endif

/* DSP config */
#define THREAD_NUM 24
static int nthreads = THREAD_NUM;                 // 申请的线程组的线程数
static int clusterId = 0;                         // 申请的cluster的ID，有0 - 3，共四个加速簇
static const char *devProgram = "./corr.dev.dat"; // 设备端二进制文件的位置
static const char *kernel0 = "mean_kernel";       // 核函数名称
static const char *kernel1 = "std_kernel";        // 核函数名称
static const char *kernel2 = "reduce_kernel";     // 核函数名称
static const char *kernel3 = "corr_kernel";       // 核函数名称

#endif /* !CORRELATION*/
