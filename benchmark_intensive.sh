#!/bin/bash

echo "========================================="
echo "Performance Comparison: Intensive Mixing"
echo "Population: 100,000 agents, 50 entities"
echo "Simulation: 100 days"
echo "========================================="

# Function to convert time to seconds
time_to_seconds() {
    local time_str="$1"
    if [[ $time_str == *m* ]]; then
        local minutes=$(echo $time_str | sed 's/m.*//g')
        local seconds=$(echo $time_str | sed 's/.*m//g' | sed 's/s//g')
        echo "$minutes * 60 + $seconds" | bc -l
    else
        echo $time_str | sed 's/s//g'
    fi
}

# Switch to master branch and compile
echo "Testing master branch..."
git checkout master > /dev/null 2>&1
g++ -std=c++17 -I include -fopenmp -O2 tests/intensive-mixing.cpp -o tests/intensive-mixing-master 2>/dev/null

if [ ! -f "tests/intensive-mixing-master" ]; then
    echo "Error: Could not compile master branch"
    exit 1
fi

echo "Running master branch 3 times:"
total_master=0
for i in {1..3}; do
    echo -n "  Run $i: "
    result=$(cd /workspaces/epiworld && { time ./tests/intensive-mixing-master > /dev/null 2>&1; } 2>&1)
    real_time=$(echo "$result" | grep "real" | awk '{print $2}')
    echo "$real_time"
    
    total_seconds=$(time_to_seconds "$real_time")
    total_master=$(echo "$total_master + $total_seconds" | bc -l)
done
avg_master=$(echo "scale=3; $total_master / 3" | bc -l)

# Switch to speedup-mixing branch and compile
echo
echo "Testing speedup-mixing branch..."
git checkout speedup-mixing > /dev/null 2>&1
g++ -std=c++17 -I include -fopenmp -O2 tests/intensive-mixing.cpp -o tests/intensive-mixing-speedup 2>/dev/null

if [ ! -f "tests/intensive-mixing-speedup" ]; then
    echo "Error: Could not compile speedup-mixing branch"
    exit 1
fi

echo "Running speedup-mixing branch 3 times:"
total_speedup=0
for i in {1..3}; do
    echo -n "  Run $i: "
    result=$(cd /workspaces/epiworld && { time ./tests/intensive-mixing-speedup > /dev/null 2>&1; } 2>&1)
    real_time=$(echo "$result" | grep "real" | awk '{print $2}')
    echo "$real_time"
    
    total_seconds=$(time_to_seconds "$real_time")
    total_speedup=$(echo "$total_speedup + $total_seconds" | bc -l)
done
avg_speedup=$(echo "scale=3; $total_speedup / 3" | bc -l)

# Calculate speedup
echo
echo "========================================="
echo "RESULTS:"
echo "========================================="
echo "Master branch average:      ${avg_master}s"
echo "Speedup-mixing average:     ${avg_speedup}s"

if (( $(echo "$avg_master > $avg_speedup" | bc -l) )); then
    speedup=$(echo "scale=2; $avg_master / $avg_speedup" | bc -l)
    improvement=$(echo "scale=1; ($avg_master - $avg_speedup) / $avg_master * 100" | bc -l)
    echo "Speedup:                    ${speedup}x faster"
    echo "Performance improvement:    ${improvement}%"
else
    slowdown=$(echo "scale=2; $avg_speedup / $avg_master" | bc -l)
    regression=$(echo "scale=1; ($avg_speedup - $avg_master) / $avg_master * 100" | bc -l)
    echo "Slowdown:                   ${slowdown}x slower"
    echo "Performance regression:     ${regression}%"
fi
echo "========================================="
