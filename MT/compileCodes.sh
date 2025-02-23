#!/bin/bash

# 定义所有可用的数据集大小
DATASETS=("MINI_DATASET" "SMALL_DATASET" "STANDARD_DATASET" "LARGE_DATASET" "EXTRALARGE_DATASET")

# 帮助信息函数
print_help() {
    echo "Usage: $0 <dataset_size>"
    echo "Dataset sizes:"
    echo "  MINI_DATASET         - Compile with mini dataset"
    echo "  SMALL_DATASET        - Compile with small dataset"
    echo "  STANDARD_DATASET     - Compile with standard dataset"
    echo "  LARGE_DATASET        - Compile with large dataset"
    echo "  EXTRALARGE_DATASET   - Compile with extra large dataset"
    echo "  ALL                  - Compile with all dataset sizes"
    exit 1
}

# 检查参数
if [ $# -ne 1 ]; then
    print_help
fi

# 验证输入参数
VALID_SIZE=0
if [ "$1" = "ALL" ]; then
    VALID_SIZE=1
else
    for size in "${DATASETS[@]}"; do
        if [ "$1" = "$size" ]; then
            VALID_SIZE=1
            break
        fi
    done
fi

if [ $VALID_SIZE -eq 0 ]; then
    echo "Error: Invalid dataset size: $1"
    print_help
fi

# 创建或清理bin目录
if [ "$1" = "ALL" ]; then
    echo "Cleaning entire bin directory..."
    rm -rf bin
    mkdir -p bin
    for size in "${DATASETS[@]}"; do
        mkdir -p "bin/$size"
    done
else
    mkdir -p bin
    echo "Cleaning bin/$1 directory..."
    rm -rf "bin/$1"
    mkdir -p "bin/$1"
fi

# 编译函数
compile_and_copy() {
    local op_dir=$1
    local dataset_size=$2
    
    # 获取可执行文件名
    local executable=$(grep "EXECUTABLE *:=" "$op_dir/Makefile" | cut -d':' -f2 | tr -d ' =')
    if [ -z "$executable" ]; then
        echo "Error: Could not find EXECUTABLE in $op_dir/Makefile"
        return 1
    fi

    echo "Compiling $op_dir for $dataset_size..."
    
    # 进入操作符目录
    cd "$op_dir" || exit 1
    
    # 清理并编译
    make clean > /dev/null
    if [ "$dataset_size" = "STANDARD_DATASET" ]; then
        make  # STANDARD_DATASET 不添加 SIZE 参数
    else
        make "SIZE=-D$dataset_size"  # 其他数据集添加宏定义
    fi
    
    if [ $? -ne 0 ]; then
        echo "Error: Compilation failed for $op_dir with $dataset_size"
        cd ..
        return 1
    fi
    
    # 创建目标目录
    mkdir -p "../bin/$dataset_size/$executable"
    
    # 复制文件
    cp "$executable" "../bin/$dataset_size/$executable/"
    cp "$executable.dev.dat" "../bin/$dataset_size/$executable/"
    
    echo "Successfully compiled and copied $executable for $dataset_size"
    
    # 返回上级目录
    cd ..
}

# 主处理逻辑
echo "Starting compilation process..."

# 遍历所有操作符目录
for op_dir in */; do
    # 跳过bin目录和非目录文件
    if [ "$op_dir" = "bin/" ] || [ "$op_dir" = "results/" ] || [ ! -d "$op_dir" ]; then
        continue
    fi
    
    if [ "$1" = "ALL" ]; then
        # 编译所有数据集大小
        for size in "${DATASETS[@]}"; do
            compile_and_copy "${op_dir%/}" "$size"
        done
    else
        # 编译指定数据集大小
        compile_and_copy "${op_dir%/}" "$1"
    fi
done

echo "Cleaning up operator directories..."
for op_dir in */; do
    # 跳过bin, results目录和非目录文件
    if [ "$op_dir" = "bin/" ] || [ "$op_dir" = "results/" ] || [ ! -d "$op_dir" ]; then
        continue
    fi
    
    # 获取可执行文件名
    executable=$(grep "EXECUTABLE *:=" "$op_dir/Makefile" | cut -d':' -f2 | tr -d ' =')
    if [ -n "$executable" ]; then
        # 删除可执行文件和.dev.dat文件
        rm -f "$op_dir$executable" "$op_dir$executable.dev.dat"
    fi
done

echo "Compilation process completed!"