/**
 * atax.cuh: This file is part of the PolyBench/GPU 1.0 test suite.
 *
 *
 * Contact: Scott Grauer-Gray <sgrauerg@gmail.com>
 * Will Killian <killian@udel.edu>
 * Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://www.cse.ohio-state.edu/~pouchet/software/polybench/GPU
 */

#ifndef ATAX_H
# define ATAX_H

/* Default to STANDARD_DATASET. */
# if !defined(MINI_DATASET) && !defined(SMALL_DATASET) && !defined(LARGE_DATASET) && !defined(EXTRALARGE_DATASET)
#  define STANDARD_DATASET
# endif

/* Do not define anything if the user manually defines the size. */
# if !defined(N)
/* Define the possible dataset sizes. */
#  ifdef MINI_DATASET
#define NX 2048
#define NY 2048
#  endif

#  ifdef SMALL_DATASET
#define NX 4096
#define NY 4096
#  endif

#  ifdef STANDARD_DATASET /* Default if unspecified. */
#define NX 8192
#define NY 8192
#  endif

#  ifdef LARGE_DATASET
#define NX 16384
#define NY 16384
#  endif

#  ifdef EXTRALARGE_DATASET
#define NX 32768
#define NY 32768
#  endif
# endif /* !N */

# define _PB_NX POLYBENCH_LOOP_BOUND(NX,nx)
# define _PB_NY POLYBENCH_LOOP_BOUND(NY,ny)

# ifndef DATA_TYPE
#  define DATA_TYPE double
#  define DATA_PRINTF_MODIFIER "%0.2lf "
# endif

/* Thread block dimensions */
#define DIM_THREAD_BLOCK_X 32
#define DIM_THREAD_BLOCK_Y 8

/* DSP config */
#define THREAD_NUM 24
static int nthreads = THREAD_NUM;                 // 申请的线程组的线程数
static int clusterId = 0;                         // 申请的cluster的ID，有0 - 3，共四个加速簇
static const char *devProgram = "./atax.dev.dat"; // 设备端二进制文件的位置
static const char *kernel1 = "atax_kernel1";        // 核函数名称
static const char *kernel2 = "atax_kernel2";        // 核函数名称

#endif /* !ATAX*/
