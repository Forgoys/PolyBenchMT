/**
 * gemm.cuh: This file is part of the PolyBench/GPU 1.0 test suite.
 *
 *
 * Contact: Scott Grauer-Gray <sgrauerg@gmail.com>
 * Will Killian <killian@udel.edu>
 * Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://www.cse.ohio-state.edu/~pouchet/software/polybench/GPU
 */

#ifndef GEMM_CUH
#define GEMM_CUH

 /* Default to STANDARD_DATASET. */
 # if !defined(MINI_DATASET) && !defined(SMALL_DATASET) && !defined(LARGE_DATASET) && !defined(EXTRALARGE_DATASET)
 #  define STANDARD_DATASET
 # endif

/* Do not define anything if the user manually defines the size. */
#if !defined(N)
/* Define the possible dataset sizes. */
#ifdef MINI_DATASET
#define NI 128
#define NJ 128
#define NK 128
#endif

#ifdef SMALL_DATASET
#define NI 256
#define NJ 256
#define NK 256
#endif

#ifdef STANDARD_DATASET /* Default if unspecified. */
#define NI 512
#define NJ 512
#define NK 512
#endif

#ifdef LARGE_DATASET
#define NI 1024
#define NJ 1024
#define NK 1024
#endif

#ifdef EXTRALARGE_DATASET
#define NI 2048
#define NJ 2048
#define NK 2048
#endif
#endif /* !N */

#define _PB_NI POLYBENCH_LOOP_BOUND(NI, ni)
#define _PB_NJ POLYBENCH_LOOP_BOUND(NJ, nj)
#define _PB_NK POLYBENCH_LOOP_BOUND(NK, nk)

#ifndef DATA_TYPE
#define DATA_TYPE double 
#define DATA_PRINTF_MODIFIER "%0.2lf "
#endif

/* DSP config */
#define THREAD_NUM 24
static int nthreads = THREAD_NUM;                 // 申请的线程组的线程数
static int clusterId = 0;                         // 申请的cluster的ID，有0 - 3，共四个加速簇
static const char *devProgram = "./gemm.dev.dat"; // 设备端二进制文件的位置
static const char *kernel = "gemm_kernel";        // 核函数名称

#endif /* !GEMM*/
