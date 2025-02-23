#include "hthread_host.h"
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

// define a small float value
#define SMALL_FLOAT_VAL 0.00000001f

#define M_logError(_FMT, ...)                                                                                          \
    do {                                                                                                               \
        fprintf(stderr, "Error : " _FMT "in %d of %s\n", __VA_ARGS__, __LINE__, __FILE__);                             \
    } while (0);

#define M_checkRetC(_RETC, _MSG)                                                                                       \
    do {                                                                                                               \
        if (_RETC != HT_SUCCESS) {                                                                                     \
            fprintf(stderr, "Failed to exec %s in %d of %s : %d\n", #_MSG, __LINE__, __FILE__, _RETC);                 \
        }                                                                                                              \
    } while (0);

#define M_checkMalloc(_PTR)                                                                                            \
    do {                                                                                                               \
        if (_PTR == NULL) {                                                                                            \
            fprintf(stderr, "Failed to malloc %s in %d of %s\n", #_PTR, __LINE__, __FILE__);                           \
        }                                                                                                              \
    } while (0);

// 将DATA_TYPE转为uint64_t，以作为参数传递给DSP
#define toRawBits(value) _Generic((value), float: floatToRawBits, double: doubleToRawBits, default: intToRawBits)(value)

uint64_t floatToRawBits(float f)
{
    union
    {
        uint32_t i;
        float f;
    } word;
    word.f = f;
    return (uint64_t)word.i;
}

uint64_t doubleToRawBits(double d)
{
    union
    {
        uint64_t i;
        double f;
    } word;
    word.f = d;
    return word.i;
}

uint64_t intToRawBits(int i) { return (uint64_t)i; }
int fileIsExist(const char *filePath) { return access(filePath, F_OK); }

int DSP_init(const char *devProgram, int nthreads)
{
    if (fileIsExist(devProgram) != 0) {
        M_logError("%s : No such file or directory", devProgram);
        return -1;
    }
    int clusterId = 0;
    int retc = 0;
    bool clusterFound = false;
    for (clusterId = 0; clusterId <= 3; clusterId++) {
        printf("Checking clusterId %d...\n", clusterId);
        retc = hthread_dev_open(clusterId);
        if (retc != 0) {
            printf("Failed to open device on clusterId %d\n", clusterId);
            continue; // 尝试下一个 clusterId
        }

        retc = hthread_dat_load(clusterId, devProgram);
        if (retc != 0) {
            printf("Failed to load data on clusterId %d\n", clusterId);
            hthread_dev_close(clusterId); // 关闭设备后继续
            continue;
        }

        int availThreads = hthread_get_avail_threads(clusterId);
        if (nthreads <= availThreads) {
            printf("Sufficient threads available on clusterId %d: available = %d, required = %d\n", clusterId,
                   availThreads, nthreads);
            clusterFound = true;
            break; // 找到合适的 clusterId，跳出循环
        } else {
            printf("Not enough threads on clusterId %d: available = %d, required = %d\n", clusterId, availThreads,
                   nthreads);
            hthread_dat_unload(clusterId);
            hthread_dev_close(clusterId); // 关闭设备后继续
        }
    }

    if (!clusterFound) {
        printf("No suitable clusterId found with enough threads. Exiting program.\n");
        return -1;
    }
    return clusterId;
}

// print performance metrics
void polybench_PM_print(const char *func_name, int cluster_id, int num_threads, double t_cpu_start, double t_cpu_end,
                        double t_dsp_start, double t_dsp_end, const char *workload_info)
{
    // 计算运行时间
    double cpu_time = t_cpu_end - t_cpu_start;
    double dsp_time = t_dsp_end - t_dsp_start;

    // 计算加速比
    double speedup = cpu_time / dsp_time;

    // 打印分隔线
    printf("\n-------------- Performance Report --------------\n");

    // 打印基本信息
    printf("Function Name: %s\n", func_name);
    printf("Cluster ID: %d\n", cluster_id);
    printf("Number of Threads: %d\n", num_threads);

    // 打印计算负载信息（如果提供）
    if (workload_info != NULL && workload_info[0] != '\0') {
        printf("Workload Info: %s\n", workload_info);
    }

    // 打印时间信息（保留4位小数）
    printf("\nExecution Time:\n");
    printf("  CPU: %.4f seconds\n", cpu_time);
    printf("  DSP: %.4f seconds\n", dsp_time);

    // 打印加速比（保留2位小数）
    printf("\nSpeedup: %.2fx\n", speedup);

    // 打印分隔线
    printf("--------------------------------------------------\n");
}

float percentDiff(double val1, double val2)
{
    // 两个值都接近零时
    if ((abs(val1) < 0.01) && (abs(val2) < 0.01)) {
        return 0.0f;
    } else {
        // 更标准的百分比差异计算
        double max_val = (abs(val1) > abs(val2)) ? abs(val1) : abs(val2);
        if (max_val < SMALL_FLOAT_VAL)
            max_val = SMALL_FLOAT_VAL; // 防止除以零

        return 100.0f * (abs(val1 - val2) / max_val);
    }
}