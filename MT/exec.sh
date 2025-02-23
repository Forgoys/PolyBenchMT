#!/bin/bash

# Enhanced Performance Testing Script

# Default configuration
DEFAULT_BIN_DIR="bin"
DATASETS=("MINI_DATASET" "SMALL_DATASET" "STANDARD_DATASET" "LARGE_DATASET" "EXTRALARGE_DATASET")
CSV_HEADER="operator,dataset,threads,cpu_time,dsp_time,speedup"

# Add timestamp directory
TIMESTAMP=$(date +"%Y-%m-%d-%H-%M")
BASE_RESULTS_DIR="results"
RESULTS_DIR="$BASE_RESULTS_DIR/$TIMESTAMP"
ALL_RESULTS_DIR="$RESULTS_DIR/all_results"

# Help information
print_help() {
    echo "Usage: $0 <dataset_size> [bin_directory]"
    echo "Dataset sizes:"
    echo "  MINI_DATASET         - Test with mini dataset"
    echo "  SMALL_DATASET        - Test with small dataset"
    echo "  STANDARD_DATASET     - Test with standard dataset"
    echo "  LARGE_DATASET        - Test with large dataset"
    echo "  EXTRALARGE_DATASET   - Test with extra large dataset"
    echo "  ALL                  - Test with all dataset sizes"
    echo "bin_directory: Optional bin directory path (default: ./bin)"
    exit 1
}

# Parameter check
if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    print_help
fi

# Parse parameters
TARGET_DATASET="$1"
BIN_DIR="${2:-$DEFAULT_BIN_DIR}"

# Validate dataset parameter
if [ "$TARGET_DATASET" != "ALL" ]; then
    VALID_SIZE=0
    for size in "${DATASETS[@]}"; do
        if [ "$TARGET_DATASET" = "$size" ]; then
            VALID_SIZE=1
            break
        fi
    done
    [ $VALID_SIZE -eq 0 ] && { echo "Error: Invalid dataset size"; print_help; }
fi

# Initialize directory structure
mkdir -p "$BASE_RESULTS_DIR"
if [ -d "$RESULTS_DIR" ]; then
    echo "Warning: Results directory for current timestamp already exists"
    read -p "Do you want to overwrite? (y/n): " confirm
    if [ "$confirm" != "y" ]; then
        echo "Aborted. Please try again in a minute."
        exit 1
    fi
    rm -rf "$RESULTS_DIR"
fi

mkdir -p "$RESULTS_DIR"
[ "$TARGET_DATASET" = "ALL" ] && mkdir -p "$ALL_RESULTS_DIR"

# Temporary file
TMP_OUTPUT="/tmp/bench_output_$$.txt"

# Enhanced performance report parsing
parse_performance() {
    if ! grep -q "Performance Report" "$TMP_OUTPUT"; then
        echo "Error: No performance report found in output"
        return 1
    fi
    local cpu_time=$(grep "CPU:" "$TMP_OUTPUT" | awk '{print $2}')
    local dsp_time=$(grep "DSP:" "$TMP_OUTPUT" | awk '{print $2}') 
    local speedup=$(grep "Speedup:" "$TMP_OUTPUT" | awk '{print $2}' | tr -d 'x')
    local threads=$(grep "Number of Threads" "$TMP_OUTPUT" | awk '{print $4}')
    
    if [ -z "$cpu_time" ] || [ -z "$dsp_time" ] || [ -z "$speedup" ] || [ -z "$threads" ]; then
        echo "Error: Failed to parse all performance metrics"
        return 1
    fi

    echo "$threads,$cpu_time,$dsp_time,$speedup"
}

# Benchmark execution function
run_benchmark() {
    local op_name="$1"
    local dataset="$2"
    local bin_path="$3"
    local global_csv="$4"
    local operator_csv="$5"
    
    echo "##################################################"
    echo "Testing: $op_name"
    echo "Dataset: $dataset"
    echo "Location: $bin_path"
    echo "##################################################"
    # Execute and capture output
    (cd "$bin_path" && ./"$op_name" 2>&1 | tee "$TMP_OUTPUT")
    
    if [ $? -eq 0 ]; then
        echo "✓ Test completed successfully"
    else
        echo "✗ Test failed"
    fi

    # Parse performance data
    local parsed_data=$(parse_performance)
    [ -z "$parsed_data" ] && return 1
    
    # Construct record
    local record="${op_name},${dataset},${parsed_data}"
    
    # Write to global CSV
    echo "$record" >> "$global_csv"
    
    # If in ALL mode, write to operator CSV
    [ -n "$operator_csv" ] && echo "$record" >> "$operator_csv"
}

# Dataset processing logic
process_dataset() {
    local dataset="$1"
    local dataset_dir="$BIN_DIR/$dataset"
    local global_csv="$RESULTS_DIR/${dataset}_result.csv"
    
    # Initialize CSV file
    [ ! -f "$global_csv" ] && echo "$CSV_HEADER" > "$global_csv"
    
    # Iterate through operators
    find "$dataset_dir" -maxdepth 1 -type d ! -path "$dataset_dir" | while read -r op_dir; do
        local op_name=$(basename "$op_dir")
        local bin_file="$op_dir/$op_name"
        
        # Validate executable
        if [ ! -x "$bin_file" ]; then
            echo "Warning: Executable not found for $op_name @ $dataset"
            continue
        fi
        
        # Prepare file paths for ALL mode
        local operator_csv=""
        [ "$TARGET_DATASET" = "ALL" ] && operator_csv="$ALL_RESULTS_DIR/${op_name}_result.csv"
        
        # Execute test
        run_benchmark "$op_name" "$dataset" "$op_dir" "$global_csv" "$operator_csv"
    done
}

# Main execution flow
main() {
    # Initialize files for ALL mode
    if [ "$TARGET_DATASET" = "ALL" ]; then
        for op_dir in "$BIN_DIR"/*/*/; do
            local op_name=$(basename "$op_dir")
            local op_csv="$ALL_RESULTS_DIR/${op_name}_result.csv"
            [ ! -f "$op_csv" ] && echo "$CSV_HEADER" > "$op_csv"
        done
    fi

    # Execute tests
    if [ "$TARGET_DATASET" = "ALL" ]; then
        for dataset in "${DATASETS[@]}"; do
            process_dataset "$dataset"
    done
    else
        process_dataset "$TARGET_DATASET"
    fi
}

# Execute main function
main

# Clean up temporary file
rm -f "$TMP_OUTPUT"

echo "##################################################"
echo " All tests completed! Results saved in:"
echo " Directory: $RESULTS_DIR"
echo " - By dataset: $RESULTS_DIR/<dataset>_result.csv"
[ "$TARGET_DATASET" = "ALL" ] && echo " - By operator: $ALL_RESULTS_DIR/<operator>_result.csv"
echo "##################################################"