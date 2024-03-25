#!/bin/bash

# Iterate over all .c files in the current directory
for cfile in *.c; do
    # Use the base name of the file without the .c extension for the output
    output="${cfile%.*}"
    echo "Compiling $cfile to $output"
    gcc "$cfile" -o "$output"
done
