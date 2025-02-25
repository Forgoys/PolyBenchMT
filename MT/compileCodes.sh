#!/bin/bash

# 定义所有可用的数据集大小
DATASETS=("MINI_DATASET" "SMALL_DATASET" "STANDARD_DATASET" "LARGE_DATASET" "EXTRALARGE_DATASET")

# 定义最大并行任务数，基于目录数量而非总任务数
if command -v nproc &> /dev/null; then
    # 如果有nproc命令
    MAX_JOBS=$(nproc)
elif [ -f /proc/cpuinfo ]; then
    # 尝试从/proc/cpuinfo获取
    MAX_JOBS=$(grep -c processor /proc/cpuinfo)
else
    # 默认值
    MAX_JOBS=16
fi

# 设定并行度上限，避免过多并行
if [ $MAX_JOBS -gt 64 ]; then
    MAX_JOBS=64
fi

echo "将使用最多 $MAX_JOBS 个并行任务进行编译（每个目录为一个任务）"

# 帮助信息函数
print_help() {
    echo "Usage: $0 <dataset_size> [jobs]"
    echo "Dataset sizes:"
    echo "  MINI_DATASET         - Compile with mini dataset"
    echo "  SMALL_DATASET        - Compile with small dataset"
    echo "  STANDARD_DATASET     - Compile with standard dataset"
    echo "  LARGE_DATASET        - Compile with large dataset"
    echo "  EXTRALARGE_DATASET   - Compile with extra large dataset"
    echo "  ALL                  - Compile with all dataset sizes"
    echo ""
    echo "Options:"
    echo "  jobs                 - Number of parallel jobs (default: available CPU cores, max 64)"
    exit 1
}

# 检查参数
if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    print_help
fi

# 如果提供了第二个参数，设置为并行任务数
if [ $# -eq 2 ]; then
    if [[ "$2" =~ ^[0-9]+$ ]]; then
        MAX_JOBS=$2
        echo "已设置并行任务数为: $MAX_JOBS"
    else
        echo "Error: 并行任务数必须是正整数"
        print_help
    fi
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

# 创建日志目录
mkdir -p compile_logs

# 编译指定目录的所有所需数据集
compile_directory() {
    local op_dir=$1
    local dataset_arg=$2
    local dir_id=$3
    local log_file="compile_logs/compile_${op_dir//\//_}_${dir_id}.log"
    
    # 获取可执行文件名
    local executable=$(grep "EXECUTABLE *:=" "$op_dir/Makefile" | cut -d':' -f2 | tr -d ' =')
    if [ -z "$executable" ]; then
        echo "[$dir_id] Error: Could not find EXECUTABLE in $op_dir/Makefile" | tee -a "$log_file"
        return 1
    fi

    # 定义要编译的数据集
    local datasets_to_compile=()
    if [ "$dataset_arg" = "ALL" ]; then
        datasets_to_compile=("${DATASETS[@]}")
    else
        datasets_to_compile=("$dataset_arg")
    fi
    
    echo "[$dir_id] Starting compilation of $op_dir for ${#datasets_to_compile[@]} dataset(s)" | tee -a "$log_file"
    
    # 进入操作符目录
    cd "$op_dir" || { 
        echo "[$dir_id] Failed to cd into $op_dir" | tee -a "../$log_file"
        return 1
    }
    
    # 依次编译每个数据集（单线程处理同一目录下的不同数据集）
    for dataset_size in "${datasets_to_compile[@]}"; do
        echo "[$dir_id] Compiling $op_dir for $dataset_size..." | tee -a "../$log_file"
        
        # 清理并编译
        make clean > /dev/null 2>&1
        if [ "$dataset_size" = "STANDARD_DATASET" ]; then
            make >> "../$log_file" 2>&1  # STANDARD_DATASET 不添加 SIZE 参数
        else
            make "SIZE=-D$dataset_size" >> "../$log_file" 2>&1  # 其他数据集添加宏定义
        fi
        
        if [ $? -ne 0 ]; then
            echo "[$dir_id] Error: Compilation failed for $op_dir with $dataset_size" | tee -a "../$log_file"
            cd ..
            return 1
        fi
        
        # 创建目标目录
        mkdir -p "../bin/$dataset_size/$executable"
        
        # 复制文件
        cp "$executable" "../bin/$dataset_size/$executable/" 2>/dev/null
        cp "$executable.dev.dat" "../bin/$dataset_size/$executable/" 2>/dev/null
        
        echo "[$dir_id] Successfully compiled and copied $executable for $dataset_size" | tee -a "../$log_file"
    done
    
    # 返回上级目录
    cd ..
    return 0
}

# 主处理逻辑
echo "Starting parallel compilation process..."

# 收集所有操作符目录
declare -a OP_DIRS
for op_dir in */; do
    # 跳过bin目录、results目录、compile_logs目录和非目录文件
    if [ "$op_dir" = "bin/" ] || [ "$op_dir" = "results/" ] || [ "$op_dir" = "compile_logs/" ] || [ ! -d "$op_dir" ]; then
        continue
    fi
    
    OP_DIRS+=("${op_dir%/}")
done

total_dirs=${#OP_DIRS[@]}
echo "找到 $total_dirs 个操作符目录需要编译"

# 并行编译所有目录（每个目录内部顺序编译其所有数据集）
process_directories() {
    local dirs=("$@")
    local running=0
    local i=0
    local pids=()
    local dir_indices=()
    
    # 跟踪已完成的目录数
    local completed=0
    
    while [ $completed -lt ${#dirs[@]} ]; do
        # 启动新任务，直到达到最大并行数或任务耗尽
        while [ $running -lt $MAX_JOBS ] && [ $i -lt ${#dirs[@]} ]; do
            compile_directory "${dirs[$i]}" "$1" "$i" &
            pids+=($!)
            dir_indices+=($i)
            running=$((running + 1))
            i=$((i + 1))
        done
        
        # 等待任何一个任务完成
        if [ $running -gt 0 ]; then
            wait -n
            
            # 找出已完成的任务并从跟踪列表中移除
            for j in "${!pids[@]}"; do
                if ! kill -0 ${pids[$j]} 2>/dev/null; then
                    unset pids[$j]
                    unset dir_indices[$j]
                    running=$((running - 1))
                    completed=$((completed + 1))
                    
                    # 显示进度
                    echo "Progress: $completed/$total_dirs directories processed"
                fi
            done
            
            # 重新索引数组
            pids=("${pids[@]}")
            dir_indices=("${dir_indices[@]}")
        fi
    done
}

# 运行并行目录处理
process_directories "$1" "${OP_DIRS[@]}"

echo "Cleaning up operator directories..."
for op_dir in */; do
    # 跳过bin, results目录和非目录文件
    if [ "$op_dir" = "bin/" ] || [ "$op_dir" = "results/" ] || [ "$op_dir" = "compile_logs/" ] || [ ! -d "$op_dir" ]; then
        continue
    fi
    
    # 获取可执行文件名
    executable=$(grep "EXECUTABLE *:=" "$op_dir/Makefile" | cut -d':' -f2 | tr -d ' =')
    if [ -n "$executable" ]; then
        # 删除可执行文件和.dev.dat文件
        rm -f "$op_dir$executable" "$op_dir$executable.dev.dat"
    fi
done

echo "Compilation logs are saved in the 'compile_logs' directory"
echo "Parallel compilation process completed!"