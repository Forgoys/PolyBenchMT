# PolyBenchMT

中文 | [English](README.md)

PolyBenchMT 是 PolyBench 基准测试套件的修改版本，专门针对 MT-3000 平台使用 hthreads 进行了适配。本项目是原始 PolyBench 套件的平台迁移，针对 MT-3000 上的并行执行进行了优化。

### 可用基准测试

#### 卷积运算
- 2DCONV：二维卷积
- 3DCONV：三维卷积

#### 线性代数
- 2MM：两次矩阵乘法
- 3MM：三次矩阵乘法
- ATAX：矩阵转置和向量乘法
- BICG：BiCGStab 线性求解器的 BiCG 子核心
- DOITGEN：多分辨率分析核心
- GEMM：矩阵乘法
- GEMVER：向量乘法和矩阵加法
- GESUMMV：标量、向量和矩阵乘法
- GRAMSCHMIDT：格拉姆-施密特分解
- LU：LU 分解
- MVT：矩阵向量乘积和转置
- SYR2K：对称秩-2K 更新
- SYRK：对称秩-K 更新

#### 数据挖掘
- CORRELATION：相关性计算
- COVARIANCE：协方差计算

#### 模板计算
- ADI：交替方向隐式求解器
- FDTD-2D：二维有限差分时域核心
- JACOBI-1D：一维雅可比模板计算
- JACOBI-2D：二维雅可比模板计算

### 编译和运行

#### 目录结构
```
.
├── bin/
│   ├── MINI_DATASET/
│   ├── SMALL_DATASET/
│   ├── STANDARD_DATASET/
│   ├── LARGE_DATASET/
│   └── EXTRALARGE_DATASET/
└── results/
    └── YYYY-MM-DD-HH-MM/
        ├── all_results/
        └── <dataset>_result.csv
```

#### 编译
编译单个数据集大小：
```bash
./compile.sh <dataset_size>
```

编译所有数据集大小：
```bash
./compile.sh ALL
```

可用的数据集大小：
- MINI_DATASET：迷你数据集
- SMALL_DATASET：小型数据集
- STANDARD_DATASET：标准数据集
- LARGE_DATASET：大型数据集
- EXTRALARGE_DATASET：超大数据集

#### 执行
运行单个数据集的基准测试：
```bash
./run.sh <dataset_size>
```

运行所有数据集：
```bash
./run.sh ALL
```

结果将保存在 `results/YYYY-MM-DD-HH-MM/` 目录中，CSV 文件包含每个算子的性能指标，包括 CPU 时间、DSP 时间和加速比。

### 修改代码

可修改的关键参数：

#### DATA_TYPE
- 默认值：float
- 可以通过修改 DATA_TYPE typedef 更改为 double

#### PERCENT_DIFF_ERROR_THRESHOLD
- 定义 DSP 和 CPU 结果之间可接受的百分比差异
- 范围：0.0-100.0
- 可以在输入代码文件中针对每个算子进行调整

修改后，运行：
```bash
make clean
make
```

### 许可证

本项目采用 BSD 3-Clause 许可证 - 查看 [LICENSE](LICENSE) 文件了解详细信息。