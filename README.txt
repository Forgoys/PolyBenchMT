README

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* PolyBench/GPU 1.0:  基于CUDA、OpenCL、HMPP和OpenACC的GPU版PolyBench基准测试套件          *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

版权所有 (c) 2012, 2013 特拉华大学
联系人：  Scott Grauer-Gray <sgrauerg@gmail.com>
          William Killian <killian@udel.edu>
          John Cavazos <cavazos@udel.edu>

本基准测试套件部分源自Louis-Noel Pouchet开发的PolyBench基准测试套件，访问地址：http://www.cse.ohio-state.edu/~pouchet/software/polybench/

若使用本作品，请引用以下论文：
Scott Grauer-Gray, Lifan Xu, Robert Searles, Sudhee Ayalasomayajula, 和 John Cavazos.  
面向GPU代码的高级语言自动调优
收录于创新并行计算会议论文集
(InPar '12), 2012.

论文下载地址：http://www.eecis.udel.edu/~grauerg/

-----------------------
* 可用基准测试：
-----------------------

卷积：
2DCONV
3DCONV

线性代数：
2MM
3MM
ATAX
BICG
DOITGEN
GEMM
GESUMMV
GRAMSCHMIDT
LU
MVT
SYR2K
SYRK

数据挖掘：
CORRELATION
COVARIANCE

模板计算：
ADI
FDTD-2D
JACOBI-1D
JACOBI-2D

CUDA、OpenCL、HMPP和OpenACC代码基于PolyBench 3.2版本开发。

--------------------------------------------------------------------------------------------
* 使用说明 - 编译/运行CUDA、OpenCL和HMPP（OpenACC单独说明见下文）：
--------------------------------------------------------------------------------------------

CUDA: 
1. 设置PATH和LD_LIBRARY_PATH环境变量指向CUDA安装目录
2. 在目标文件夹运行"make"命令生成可执行文件
3. 运行生成的.exe文件

OpenCL:
1. 设置PATH和LD_LIBRARY_PATH环境变量指向OpenCL安装目录
2. 在OpenCL文件夹的common.mk文件中设置SDK路径
3. 在目标文件夹运行"make"命令生成可执行文件
4. 运行生成的.exe文件

HMPP:
1. 确保当前使用bash shell
2. 设置PATH和LD_LIBRARY_PATH指向CUDA/OpenCL安装目录 
3. 在{HMPP_INSTALLATION}/bin目录执行source hmpp-env.sh设置HMPP环境变量
4. 在目标文件夹运行"make exe"生成可执行文件
5. 运行生成的.exe文件。若运行报错，可尝试在文件夹中使用"make"或"make run"命令运行

------------------
* 修改代码说明：
------------------

可通过修改代码中的常量调整输入尺寸、数据类型和GPU-CPU输出对比阈值等参数。修改后需运行"make clean"和"make"使改动生效。

参数说明：

DATA_TYPE:
默认使用float类型，可通过修改DATA_TYPE为"double"改为双精度

PERCENT_DIFF_ERROR_THRESHOLD:
该阈值（0.0-100.0）表示GPU和CPU结果允许的最大百分比差异，可在各测试代码文件中调整

=======================================================================================================================================================================================
OpenACC 信息：

** 使用HMPP Workbench/CAPS编译器编译OpenACC版本：
-------------------------------------------------------------------

* 面向CUDA：

$> hmpp --codelet-required --openacc-target=CUDA gcc -O2 -I./utilities -I./linear-algebra/kernels/gemm/gemm utilities/polybench.c linear-algebra/kernels/gemm/gemm.c -o gemm_acc
   
   或
   
$> capsmc --codelet-required --openacc-target=CUDA gcc -O2 -I./utilities -I./linear-algebra/kernels/gemm/gemm utilities/polybench.c linear-algebra/kernels/gemm/gemm.c -o gemm_acc

* 面向OpenCL：

$> hmpp --codelet-required --openacc-target=OPENCL gcc -O2 -I./utilities -I./linear-algebra/kernels/gemm/gemm utilities/polybench.c linear-algebra/kernels/gemm/gemm.c -o gemm_acc
   
   或
   
$> capsmc --codelet-required --openacc-target=OPENCL gcc -O2 -I./utilities -I./linear-algebra/kernels/gemm/gemm utilities/polybench.c linear-algebra/kernels/gemm/gemm.c -o gemm_acc

** 生成基准测试参考输出：
---------------------------------------------------

* 编译时给主机编译器（gcc或icc）传递-DPOLYBENCH_DUMP_ARRAYS参数

$> ./gemm_acc 2>gemm_ref.out

------------------------------------
* OpenACC可用选项：
------------------------------------

通过编译时宏定义传递（例如-D选项名）

- POLYBENCH_TIME: 输出执行时间（使用gettimeofday）[默认关闭]

- POLYBENCH_NO_FLUSH_CACHE: 不刷新缓存[默认刷新]

- POLYBENCH_LINUX_FIFO_SCHEDULER: 使用FIFO实时调度策略（需root权限，仅限Linux）[默认关闭]

- POLYBENCH_CACHE_SIZE_KB: 缓存刷新大小（单位KB）[默认33MB]

- POLYBENCH_STACK_ARRAYS: 使用栈分配替代malloc[默认关闭]

- POLYBENCH_DUMP_ARRAYS: 输出所有存活数组[默认关闭]

- POLYBENCH_CYCLE_ACCURATE_TIMER: 使用时间戳计数器计时[默认关闭]

- POLYBENCH_PAPI: 启用PAPI计时（见下文）

- MINI_DATASET, SMALL_DATASET, STANDARD_DATASET, LARGE_DATASET,
  EXTRALARGE_DATASET: 设置数据集大小[默认STANDARD_DATASET]

- POLYBENCH_USE_C99_PROTO: 使用C99标准函数原型

- POLYBENCH_USE_SCALAR_LB: 使用标量循环边界替代参数化边界

--------------------------
* PAPI支持（OpenACC）：
--------------------------

** 编译支持PAPI的基准测试：
--------------------------------------------

$> gcc -O3 -I utilities -I linear-algebra/kernels/atax utilities/polybench.c linear-algebra/kernels/atax/atax.c -DPOLYBENCH_PAPI -lpapi -o atax_papi

** 指定监控的计数器：
------------------------------------------

编辑utilities/papi_counters.list文件，每行添加一个监控事件（包括最后一行）需以逗号结尾，支持原生和标准事件。

-----------------------------------------
* 精确性能计时（OpenACC）：
-----------------------------------------

$> gcc -O3 -I utilities -I linear-algebra/kernels/atax utilities/polybench.c linear-algebra/kernels/atax/atax.c -DPOLYBENCH_TIME -o atax_time
$> ./utilities/time_benchmark.sh ./atax_time

该脚本将运行五次测试，去除极值后检查剩余三次的偏差是否超过5%阈值。

---------------------------------------------------
* 生成无宏基准测试套件（OpenACC）：
---------------------------------------------------

（在根目录执行：）
$> PARGS="-I utilities -DPOLYBENCH_TIME";
$> for i in `cat utilities/benchmark_list`; do create_cpped_version.sh $i "$PARGS"; done

====================================================================================================================================================================================

---------------
* 联系方式： 
---------------

联系人：Scott Grauer-Gray <sgrauerg@gmail.com>
        Will Killian <killian@udel.edu>
        John Cavazos <cavazos@udel.edu>

------------------------
* 相关论文： 
------------------------

Scott Grauer-Gray, Lifan Xu, Robert Searles, Sudhee Ayalasomayajula, 和 John Cavazos.  
面向GPU代码的高级语言自动调优
收录于创新并行计算会议论文集 
(InPar '12), 2012.

致谢：
本工作部分由美国国家科学基金会（NSF Career award 0953667）和DARPA计算机科学研究组（CSSG）资助。