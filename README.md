# PolyBenchMT

[中文](README_zh.md) | English

PolyBenchMT is a modified version of PolyBench benchmarks, specifically adapted for the MT-3000 platform using hthreads. This project represents a platform migration of the original PolyBench suite, optimized for parallel execution on MT-3000.

### Available Benchmarks

#### Convolution
- 2DCONV: 2D Convolution
- 3DCONV: 3D Convolution

#### Linear Algebra
- 2MM: 2 Matrix Multiplications
- 3MM: 3 Matrix Multiplications
- ATAX: Matrix Transpose and Vector Multiplication
- BICG: BiCG Sub Kernel of BiCGStab Linear Solver
- DOITGEN: Multiresolution Analysis Kernel
- GEMM: Matrix-Multiply
- GEMVER: Vector Multiplication and Matrix Addition
- GESUMMV: Scalar, Vector and Matrix Multiplication
- GRAMSCHMIDT: Gram-Schmidt Decomposition
- LU: LU Decomposition
- MVT: Matrix Vector Product and Transpose
- SYR2K: Symmetric Rank-2K Update
- SYRK: Symmetric Rank-K Update

#### Datamining
- CORRELATION: Correlation Computation
- COVARIANCE: Covariance Computation

#### Stencils
- ADI: Alternating Direction Implicit Solver
- FDTD-2D: 2D Finite Different Time Domain Kernel
- JACOBI-1D: 1D Jacobi Stencil Computation
- JACOBI-2D: 2D Jacobi Stencil Computation

### Building and Running

#### Directory Structure
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

#### Compilation
To compile a single dataset size:
```bash
./compile.sh <dataset_size>
```

To compile all dataset sizes:
```bash
./compile.sh ALL
```

Available dataset sizes:
- MINI_DATASET
- SMALL_DATASET
- STANDARD_DATASET
- LARGE_DATASET
- EXTRALARGE_DATASET

#### Execution
To run benchmarks for a single dataset:
```bash
./run.sh <dataset_size>
```

To run all datasets:
```bash
./run.sh ALL
```

The results will be saved in the `results/YYYY-MM-DD-HH-MM/` directory, with CSV files containing performance metrics including CPU time, DSP time, and speedup for each operator.

### Modifying Codes

Key parameters that can be modified:

#### DATA_TYPE
- Default: float
- Can be changed to double by modifying the DATA_TYPE typedef

#### PERCENT_DIFF_ERROR_THRESHOLD
- Defines the acceptable percentage difference between DSP and CPU results
- Range: 0.0-100.0
- Can be adjusted per operator in the input code file

After modifications, run:
```bash
make clean
make
```

### License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.