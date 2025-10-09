#!/bin/bash

DIR="/usr/lib/gcc/x86_64-linux-gnu/11/include/"

# Check if directory exists
if [ ! -d "$DIR" ]; then
    echo "Error: Directory $DIR does not exist"
    exit 1
fi

# Check if lex executable exists
if [ ! -x "./lex" ]; then
    echo "Error: ./lex executable not found or not executable"
    exit 1
fi

# Initialize variables for statistics
total_files=0
total_velocity=0
max_velocity=0
min_velocity=-1
max_velocity_file=""
min_velocity_file=""

# Process each file in the directory
for file in "$DIR"*; do
    # Check if it's a regular file (not a directory)
    if [ -f "$file" ]; then
        filename=$(basename "$file")
        echo "Processing: $filename"
        
        # Get file size using wc
        size=$(wc -c < "$file")
        
        # Get runtime using time command
        TIMEFORMAT='%3R'
        runtime=$( { time ./lex "$file" > /dev/null; } 2>&1 )
        
        # Calculate velocity (size/runtime)
        # Use bc for floating point division, handle division by zero
        if (( $(echo "$runtime > 0" | bc -l) )); then
            velocity=$(echo "scale=2; $size / $runtime" | bc)
        else
            velocity=0
        fi
        
        # Print the results for current file
        echo "Size: $size bytes, Runtime: $runtime seconds, Velocity: $velocity bytes/sec"
        echo "---"
        
        # Update statistics
        total_files=$((total_files + 1))
        total_velocity=$(echo "scale=2; $total_velocity + $velocity" | bc)
        
        # Update max velocity
        if (( $(echo "$velocity > $max_velocity" | bc -l) )); then
            max_velocity=$velocity
            max_velocity_file=$filename
        fi
        
        # Update min velocity (initialize on first file)
        if (( $(echo "$min_velocity == -1" | bc -l) )) || (( $(echo "$velocity < $min_velocity" | bc -l) )); then
            min_velocity=$velocity
            min_velocity_file=$filename
        fi
    fi
done

# Calculate average velocity
if [ $total_files -gt 0 ]; then
    average_velocity=$(echo "scale=2; $total_velocity / $total_files" | bc)
else
    average_velocity=0
fi

# Print summary statistics
echo "=========================================="
echo "SUMMARY STATISTICS"
echo "=========================================="
echo "Total files processed: $total_files"
echo "Average velocity: $average_velocity bytes/sec"
echo "Maximum velocity: $max_velocity bytes/sec (file: $max_velocity_file)"
echo "Minimum velocity: $min_velocity bytes/sec (file: $min_velocity_file)"
