#ifndef TWOMM_H
# define TWOMM_H

/* Default to STANDARD_DATASET. */
# if !defined(MINI_DATASET) && !defined(SMALL_DATASET) && !defined(LARGE_DATASET) && !defined(EXTRALARGE_DATASET)
#  define STANDARD_DATASET
# endif

/* Do not define anything if the user manually defines the size. */
# if !defined(N)
/* Define the possible dataset sizes. */
#  ifdef MINI_DATASET
# define NI 384
# define NJ 384
# define NK 384
# define NL 384
#  endif

#  ifdef SMALL_DATASET
# define NI 512
# define NJ 512
# define NK 512
# define NL 512
#  endif

#  ifdef STANDARD_DATASET /* Default if unspecified. */
# define NI 640
# define NJ 640
# define NK 640
# define NL 640
#  endif

#  ifdef LARGE_DATASET
# define NI 768
# define NJ 768
# define NK 768
# define NL 768
#  endif

#  ifdef EXTRALARGE_DATASET
# define NI 896
# define NJ 896
# define NK 896
# define NL 896
#  endif
# endif /* !N */

# define _PB_NI POLYBENCH_LOOP_BOUND(NI,ni)
# define _PB_NJ POLYBENCH_LOOP_BOUND(NJ,nj)
# define _PB_NK POLYBENCH_LOOP_BOUND(NK,nk)
# define _PB_NL POLYBENCH_LOOP_BOUND(NL,nl)

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
static const char *devProgram = "./2mm.dev.dat"; // 设备端二进制文件的位置
static const char *kernel1 = "mm2_kernel1";        // 核函数名称
static const char *kernel2 = "mm2_kernel2";        // 核函数名称


#endif /* !TWOMM*/