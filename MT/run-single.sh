#!/bin/bash
if [ $# -ne 1 ]; then
    echo "Usage: $0 <executable_path>"
    exit 1
fi
EXECUTABLE="$1"
JOB_SCRIPT="job_script.sh"
OUTPUT_FILE="slurm_job_output.txt"

# Check if the executable file exists
if [ ! -x "$EXECUTABLE" ]; then
    echo "Error: $EXECUTABLE does not exist or is not executable"
    exit 1
fi

# Create job script with output directed to a file
cat > "$JOB_SCRIPT" << EOF
#!/bin/bash
#SBATCH -N 1                # Use 1 node
#SBATCH -n 16               # Use 16 processes
#SBATCH -p thmt1            # Use thmt1 queue
#SBATCH --output=$OUTPUT_FILE # Direct output to a file
$EXECUTABLE
EOF

# Submit job and wait for completion
echo "Submitting job..."
JOB_ID=$(sbatch "$JOB_SCRIPT" | awk '{print $NF}')
echo "Job ID: $JOB_ID"

# Wait for job to complete
echo "Waiting for job to complete..."
while true; do
    STATUS=$(squeue -j "$JOB_ID" 2>/dev/null | wc -l)
    if [ "$STATUS" -eq 1 ]; then  # Only header means job has ended
        break
    fi
    sleep 5
done

# Display job output
if [ -f "$OUTPUT_FILE" ]; then
    echo "======================== JOB OUTPUT ========================"
    cat "$OUTPUT_FILE"
    echo "======================== END OUTPUT ========================"
fi

# Check if the job completed successfully
if [ -f "slurm-${JOB_ID}.err" ] && [ -s "slurm-${JOB_ID}.err" ]; then
    echo "Job execution failed, error message:"
    cat "slurm-${JOB_ID}.err"
    EXIT_CODE=1
else
    echo "Job completed successfully"
    EXIT_CODE=0
fi

# Clean up temporary files
rm -f "$JOB_SCRIPT" "$OUTPUT_FILE" "slurm-${JOB_ID}.err"
exit $EXIT_CODE