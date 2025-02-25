#ifndef DOITGEN_H
#define DOITGEN_H

/* Default to STANDARD_DATASET. */
#if !defined(MINI_DATASET) && !defined(SMALL_DATASET) && !defined(LARGE_DATASET) && !defined(EXTRALARGE_DATASET)
#define STANDARD_DATASET
#endif

/* Do not define anything if the user manually defines the size. */
#if !defined(NQ) && !defined(NR) && !defined(NP)
/* Define the possible dataset sizes. */
#ifdef MINI_DATASET
#define NQ 32
#define NR 32
#define NP 32
#endif

#ifdef SMALL_DATASET
#define NQ 64
#define NR 64
#define NP 64
#endif

#ifdef STANDARD_DATASET /* Default if unspecified. */
#define NQ 128
#define NR 128
#define NP 128
#endif

#ifdef LARGE_DATASET
#define NQ 256
#define NR 256
#define NP 256
#endif

#ifdef EXTRALARGE_DATASET
#define NQ 512
#define NR 512
#define NP 512
#endif
#endif /* !N */

#define _PB_NQ POLYBENCH_LOOP_BOUND(NQ, nq)
#define _PB_NR POLYBENCH_LOOP_BOUND(NR, nr)
#define _PB_NP POLYBENCH_LOOP_BOUND(NP, np)

#ifndef DATA_TYPE
#define DATA_TYPE double
#define DATA_PRINTF_MODIFIER "%0.2lf "
#endif

/* DSP config */
#define THREAD_NUM 24
static int nthreads = THREAD_NUM;                    // 申请的线程组的线程数
static int clusterId = 0;                            // 申请的cluster的ID，有0 - 3，共四个加速簇
static const char *devProgram = "./doitgen.dev.dat"; // 设备端二进制文件的位置
static const char *kernel1 = "doitgen_kernel1";      // 核函数名称
static const char *kernel2 = "doitgen_kernel2";      // 核函数名称

#endif /* !DOITGEN */
