/**
 * jacobi1D.cuh: This file is part of the PolyBench/GPU 1.0 test suite.
 *
 *
 * Contact: Scott Grauer-Gray <sgrauerg@gmail.com>
 * Will Killian <killian@udel.edu>
 * Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://www.cse.ohio-state.edu/~pouchet/software/polybench/GPU
 */

 #ifndef JACOBI1D_H
 # define JACOBI1D_H
 
 /* Default to STANDARD_DATASET. */
 # if !defined(MINI_DATASET) && !defined(SMALL_DATASET) && !defined(LARGE_DATASET) && !defined(EXTRALARGE_DATASET)
 #  define STANDARD_DATASET
 # endif

 /* Do not define anything if the user manually defines the size. */
 # if !defined(N)
 /* Define the possible dataset sizes. */
 #  ifdef MINI_DATASET
 #define TSTEPS 10000
 #define N 1024
 #  endif
 
 #  ifdef SMALL_DATASET
 #define TSTEPS 10000
 #define N 2048
 #  endif
 
 #  ifdef STANDARD_DATASET /* Default if unspecified. */
 #define TSTEPS 10000
 #define N 4096
 #  endif
 
 #  ifdef LARGE_DATASET
 #define TSTEPS 10000
 #define N 8192
 #  endif
 
 #  ifdef EXTRALARGE_DATASET
 #define TSTEPS 10000
 #define N 16384
 #  endif
 # endif /* !N */
 
 # define _PB_TSTEPS POLYBENCH_LOOP_BOUND(TSTEPS,tsteps)
 # define _PB_N POLYBENCH_LOOP_BOUND(N,n)
 
 # ifndef DATA_TYPE
 #  define DATA_TYPE float
 #  define DATA_PRINTF_MODIFIER "%0.2lf "
 # endif
 
 /* Thread block dimensions */
 #define DIM_THREAD_BLOCK_X 256
 #define DIM_THREAD_BLOCK_Y 1
 
 /* DSP config */
#define THREAD_NUM 24
static int nthreads = THREAD_NUM;                 // 申请的线程组的线程数
static int clusterId = 0;                         // 申请的cluster的ID，有0 - 3，共四个加速簇
static const char *devProgram = "./jacobi1D.dev.dat"; // 设备端二进制文件的位置
static const char *kernel1 = "runJacobiDSP_kernel1";        // 核函数名称
static const char *kernel2 = "runJacobiDSP_kernel2";        // 核函数名称
 
 #endif /* !JACOBI1D*/