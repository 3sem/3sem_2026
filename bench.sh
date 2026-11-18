#!/bin/bash
POINTS=100000000 # 100 миллионов точек

echo "Starting benchmark with $POINTS points..."
echo "Threads | Time (s) | Calculated Integral"
echo "-----------------------------------------"

for t in 1 2 4 8 16 32 64; do
    OUT=$(./progA $POINTS $t)
    TIME=$(echo "$OUT" | awk -F'Time: ' '{print $2}' | awk '{print $1}')
    
    INTEGRAL=$(./progB | awk -F'Calculated Integral: ' '{print $2}' | awk '{print $1}')
    
    echo "$t | $TIME | $INTEGRAL"
done