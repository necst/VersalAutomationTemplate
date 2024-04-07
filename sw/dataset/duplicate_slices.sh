#!/bin/bash

# Set the base filename
base="IM"

# Set the starting number
start=$1

# Set the number of copies
copies=$2

# Loop through the copies and copy the file
for ((i=$start; i<$start+$copies-1; i++)); do
    cp "${base}1.png" "${base}${i}.png"
done
