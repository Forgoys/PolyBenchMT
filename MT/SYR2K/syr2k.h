
#ifndef SYR2K_H
# define SYR2K_H

/* Default to STANDARD_DATASET. */
# if !defined(MINI_DATASET) && !defined(SMALL_DATASET) && !defined(LARGE_DATASET) && !defined(EXTRALARGE_DATASET)
#  define STANDARD_DATASET
# endif

/* Do not define anything if the user manually defines the size. */
# if !defined(N)
/* Define the possible dataset sizes. */
#  ifdef MINI_DATASET
#define NI 128
#define NJ 128
#  endif

#  ifdef SMALL_DATASET
#define NI 256
#define NJ 256
#  endif

#  ifdef STANDARD_DATASET /* Default if unspecified. */
#define NI 512
#define NJ 512
#  endif

#  ifdef LARGE_DATASET
#define NI 1024
#define NJ 1024
#  endif

#  ifdef EXTRALARGE_DATASET
#define NI 2048
#define NJ 2048
#  endif
# endif /* !N */

# define _PB_NI POLYBENCH_LOOP_BOUND(NI,ni)
# define _PB_NJ POLYBENCH_LOOP_BOUND(NJ,nj)

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
static const char *devProgram = "./syr2k.dev.dat"; // 设备端二进制文件的位置
static const char *kernel = "syr2k_kernel";        // 核函数名称
#endif /* !SYR2K*/